// file      : evolution/drop-index/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test dropping an index.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

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

        {
          transaction t (db->begin ());
          db->persist (o0);
          db->persist (o1);
          t.commit ();
        }

        // Duplicates are not ok.
        //
        try
        {
          transaction t (db->begin ());
          db->persist (o2);
          assert (false);
        }
        catch (const odb::exception& ) {}

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

        // Duplicates are now ok.
        //
        object o2 (2);
        o2.num = 234;

        {
          transaction t (db->begin ());
          db->persist (o2);
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
          auto_ptr<object> p0 (db->load<object> (0));
          auto_ptr<object> p1 (db->load<object> (1));
          auto_ptr<object> p2 (db->load<object> (2));

          assert (p0->num == 123);
          assert (p1->num == 234);
          assert (p2->num == 234);

          t.commit ();
        }

        // Duplicates are still ok.
        //
        object o3 (3);
        o3.num = 234;

        {
          transaction t (db->begin ());
          db->persist (o3);
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
