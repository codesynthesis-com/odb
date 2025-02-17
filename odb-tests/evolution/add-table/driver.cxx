// file      : evolution/add-table/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test adding a new table (object, container).
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <libcommon/config.hxx>  // DATABASE_XXX
#include <libcommon/common.hxx>

#include "test2.hxx"
#include "test3.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv, false));

    db->schema_version_table (quote_name ("evo_add_t_sv"));

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
          // SQLite has broken foreign keys when it comes to DDL.
          //
#ifdef DATABASE_SQLITE
          db->connection ()->execute ("PRAGMA foreign_keys=OFF");
#endif
          transaction t (db->begin ());
          schema_catalog::drop_schema (*db);
          schema_catalog::create_schema (*db, "", false);
          schema_catalog::migrate_schema (*db, 2);
          t.commit ();

#ifdef DATABASE_SQLITE
          db->connection ()->execute ("PRAGMA foreign_keys=ON");
#endif
        }

        object o (1);
        o.str = "abc";

        {
          transaction t (db->begin ());
          db->persist (o);
          t.commit ();
        }
        break;
      }
    case 2:
      {
        using namespace v3;

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_pre (*db, 3);
          t.commit ();
        }

        object1 o1;
        o1.nums.push_back (1);
        o1.nums.push_back (2);
        o1.nums.push_back (3);

        {
          transaction t (db->begin ());
          o1.o = db->load<object> (1);
          db->persist (o1);
          t.commit ();
        }

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_post (*db, 3);
          t.commit ();
        }
        break;
      }
    case 3:
      {
        using namespace v3;

        typedef odb::query<object1> query;
        typedef odb::result<object1> result;

        {
          transaction t (db->begin ());

          result r (db->query<object1> (query::o->str == "abc"));
          result::iterator i (r.begin ()), e (r.end ());

          assert (i != e &&
                  i->o->id_ == 1 &&
                  i->nums[0] == 1 && i->nums[1] == 2 && i->nums[2] == 3);
          assert (++i == e);

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
