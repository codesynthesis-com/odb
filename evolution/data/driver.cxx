// file      : evolution/data/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test data migration support.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <common/config.hxx>  // DATABASE_XXX, HAVE_CXX11
#include <common/common.hxx>

#include "test2.hxx"
#include "test3.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"

using namespace std;
using namespace odb::core;

void
migrate1 (database& db)
{
  using namespace v2;
  using namespace v3;

  auto_ptr<object1> o1 (db.load<object1> (1));
  object2 o2 (1);
  o2.num = o1->num;
  db.persist (o2);
}

void
migrate2 (database& db)
{
  using namespace v2;
  using namespace v3;

  auto_ptr<object1> o1 (db.load<object1> (2));
  object2 o2 (2);
  o2.num = o1->num;
  db.persist (o2);
}

static const data_migration_entry<3, 1> migrate2_entry (&migrate2);

int
main (int argc, char* argv[])
{
  schema_catalog::data_migration_function<3, 1> (&migrate1);

#ifdef HAVE_CXX11
  schema_catalog::data_migration_function<3, 1> (
    [] (database& db)
    {
      using namespace v2;
      using namespace v3;

      auto_ptr<object1> o1 (db.load<object1> (11));
      object2 o2 (11);
      o2.num = o1->num;
      db.persist (o2);
    });
#endif

  try
  {
    auto_ptr<database> db (create_database (argc, argv, false));
    bool embedded (schema_catalog::exists (*db));

    // 1 - base version
    // 2 - migration
    // 3 - current version
    //
    unsigned short pass (*argv[argc - 1] - '0');

    switch (pass)
    {
    case 1:
      {
        using namespace v2;

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::drop_schema (*db);
          schema_catalog::create_schema (*db, "", false);
          schema_catalog::migrate_schema (*db, 2);
          t.commit ();
        }

        {
          transaction t (db->begin ());

          {
            object1 o1 (1);
            o1.num = 123;
            db->persist (o1);
          }

          {
            object1 o1 (2);
            o1.num = 123;
            db->persist (o1);
          }

#ifdef HAVE_CXX11
          {
            object1 o1 (11);
            o1.num = 123;
            db->persist (o1);
          }
#endif
          t.commit ();
        }
        break;
      }
    case 2:
      {
        {
          transaction t (db->begin ());

          if (embedded)
            schema_catalog::migrate (*db);
          else
            schema_catalog::migrate_data (*db);

          t.commit ();
        }

        break;
      }
    case 3:
      {
        using namespace v3;

        {
          transaction t (db->begin ());

          {
            auto_ptr<object2> o2 (db->load<object2> (1));
            assert (o2->num == 123);
          }

          {
            auto_ptr<object2> o2 (db->load<object2> (2));
            assert (o2->num == 123);
          }

#ifdef HAVE_CXX11
          {
            auto_ptr<object2> o2 (db->load<object2> (11));
            assert (o2->num == 123);
          }
#endif
          t.commit ();
        }

        break;
      }
    default:
      {
        cerr << "unknown pass number '" << argv[argc - 1] << "'" << endl;
        return 1;
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
