// file      : evolution/add-index/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test adding a new index.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

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

    db->schema_version_table (quote_name ("evo_add_i_sv"));

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

        object o0 (0);
        o0.num = 123;

        object o1 (1);
        o1.num = 234;

        object o2 (2);
        o2.num = 234;

        // Duplicates are ok.
        //
        {
          transaction t (db->begin ());
          db->persist (o0);
          db->persist (o1);
          db->persist (o2);
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

        object o3 (3);
        o3.num = 234;

        // Duplicates are still ok but we need to remove them before the
        // post migration step.
        //
        {
          transaction t (db->begin ());
          db->persist (o3);
          t.commit ();
        }

        {
          typedef odb::query<object> query;
          typedef odb::result<object> result;

          transaction t (db->begin ());
          result r (db->query<object> (
                      "ORDER BY" + query::num + "," + query::id));

          unsigned long prev (0);
          for (result::iterator i (r.begin ()); i != r.end (); ++i)
          {
            if (i->num == prev)
              db->erase (*i);

            prev = i->num;
          }

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

        {
          transaction t (db->begin ());
          unique_ptr<object> p0 (db->load<object> (0));
          unique_ptr<object> p1 (db->load<object> (1));

          assert (p0->num == 123);
          assert (p1->num == 234);

          t.commit ();
        }

        try
        {
          object o2 (2);
          o2.num = 234;

          transaction t (db->begin ());
          db->persist (o2);
          assert (false);
        }
        catch (const odb::exception& ) {}

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
