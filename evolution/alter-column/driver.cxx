// file      : evolution/alter-column/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test altering a column.
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

    // SQLite doesn't support altering of columns.
    //
#ifndef DATABASE_SQLITE
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

        object o (1);
        o.num = 123;

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

        // NULL is already in effect; NOT NULL is not yet.
        //
        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> (1));

          assert (!p->str);
          assert (p->num && *p->num == 123);
          assert (!p->num1);

          // Migration.
          //
          p->str = "abc";
          p->num.reset ();
          p->num1 = 123;
          db->update (*p);

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
          auto_ptr<object> p (db->load<object> (1));

          assert (p->str && *p->str == "abc");
          assert (!p->num);

          t.commit ();
        }

        // NOT NULL is now in effect.
        //
        try
        {
          object o2 (2);
          o2.num1 = 234; // str is NULL

          transaction t (db->begin ());
          db->persist (o2);
          assert (false);
        }
        catch (const odb::exception& ) {}

        try
        {
          object o3 (3);
          o3.str = "bcd"; // num1 is NULL

          transaction t (db->begin ());
          db->persist (o3);
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
#endif // DATABASE_SQLITE
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
