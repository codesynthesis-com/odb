// file      : evolution/add-foreign-key/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test adding a foreign key.
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

#ifdef DATABASE_SQLITE
        // In SQLite we can only add foreign keys inline in the column
        // definition.
        //
        db->connection ()->execute ("PRAGMA foreign_keys=OFF");
#endif

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_pre (*db, 3);
          t.commit ();
        }

        // Both pointers are now NULL.
        //
        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> (1));

          assert (p->o1 == 0 && p->o2 == 0);

          // Migration. The foreign key constraint is not yet there.
          //
          p->o1 = new object1 (1);
          p->o2 = new object2 (1);
          db->update (*p);

          t.commit ();
        }

        // Migration. Add the missing objects.
        //
        object1 o1 (1);
        object2 o2 (1);

        {
          transaction t (db->begin ());
          db->persist (o1);
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
          auto_ptr<object> p (db->load<object> (1));
          assert (p->o1->id_ == 1);
          assert (p->o2->id_ == 1);
          t.commit ();
        }

        // Now the foreign key constraint is there.
        //
        try
        {
          object o (2);
          o.o1 = new object1 (2);
          o.o2 = new object2 (2);

          transaction t (db->begin ());
          db->persist (o);
          assert (false);
        }
        catch (const odb::exception& ) {}

        // As well as the NOT NULL.
        //
#ifndef DATABASE_SQLITE
        try
        {
          object o (3);
          o.o2 = 0;

          transaction t (db->begin ());
          db->persist (o);
          assert (false);
        }
        catch (const odb::exception& ) {}
#endif
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
