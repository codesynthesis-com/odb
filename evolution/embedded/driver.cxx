// file      : evolution/embedded/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test embedded schema migration.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <common/config.hxx>  // DATABASE_XXX
#include <common/common.hxx>

#include "test2.hxx"
#include "test3.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv, false));

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

        {
          transaction t (db->begin ());
          schema_catalog::drop_schema (*db);
          t.commit ();
        }

        // PostgreSQL cannot continue a transaction after a query failed.
        //
        assert (db->schema_version () == 0);

        {
          transaction t (db->begin ());
          schema_catalog::create_schema (*db, "", false);

          assert (db->schema_version () == 1 && !db->schema_migration ());

          schema_catalog::migrate_schema (*db, 2);
          t.commit ();
        }

        assert (db->schema_version () == 2 && !db->schema_migration ());

        {
          transaction t (db->begin ());
          object1 o1 (1);
          o1.num = 123;
          db->persist (o1);
          t.commit ();
        }
        break;
      }
    case 2:
      {
        using namespace v2;
        using namespace v3;

        // Check version information correctness.
        //
        assert (schema_catalog::base_version (*db) == 1);
        assert (schema_catalog::current_version (*db) == 3);
        assert (schema_catalog::next_version (*db, 0) == 3);
        assert (schema_catalog::next_version (*db, 1) == 2);
        assert (schema_catalog::next_version (*db) == 3);
        assert (schema_catalog::next_version (*db, 3) == 4);

        {
          assert (db->schema_version () == 2 && !db->schema_migration ());

          transaction t (db->begin ());
          schema_catalog::migrate_schema_pre (*db, 3);
          t.commit ();
        }

        assert (db->schema_version () == 3 && db->schema_migration ());

        {
          transaction t (db->begin ());
          auto_ptr<object1> o1 (db->load<object1> (1));
          object2 o2 (1);
          o2.num = o1->num;
          db->persist (o2);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_post (*db, 3);
          t.commit ();

          assert (db->schema_version () == 3 && !db->schema_migration ());
        }
        break;
      }
    case 3:
      {
        using namespace v3;

        // In transaction.
        //
        {
          transaction t (db->begin ());
          assert (db->schema_version () == 3 && !db->schema_migration ());
          t.commit ();
        }

        {
          transaction t (db->begin ());
          auto_ptr<object2> o2 (db->load<object2> (1));
          assert (o2->num == 123);
          t.commit ();
        }

        // Test the case where there is still no version table.
        //
        db->schema_version_migration (0, false);

        {
          transaction t (db->begin ());

#ifdef DATABASE_ORACLE
          db->execute ("DROP TABLE \"schema_version\"");
#else
          db->execute ("DROP TABLE schema_version");
#endif
          t.commit ();
        }

        assert (db->schema_version () == 0);
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
