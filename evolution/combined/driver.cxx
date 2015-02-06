// file      : evolution/combined/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Combined schema evolution test.
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

        object o ("1");
        o.dui = 1;
        o.anui = 1;
        o.dnui = 1;
        o.dc = 1;
        o.dt.push_back (1);
        o.aui = 1;

#ifndef DATABASE_SQLITE
        o.dfk = new object1 (1);
        o.acn = 1;
        o.acnn.reset ();
        o.afk = 1;
#endif

        {
          transaction t (db->begin ());
#ifndef DATABASE_SQLITE
          db->persist (o.dfk);
#endif
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

        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> ("1"));

          assert (p->ac1 == 999);
          assert (!p->ac2);

#ifndef DATABASE_SQLITE
          assert (!p->ac3);
#endif
          // Migrate.
          //
          p->at.push_back ("abc");

#ifndef DATABASE_SQLITE
          p->dfk = 999;
          p->ac3 = 1;
          p->acn.reset ();
          p->acnn = 1;
#endif
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
          auto_ptr<object> p (db->load<object> ("1"));

          // Check post-migration.
          //
          assert (p->at[0] == "abc");

#ifndef DATABASE_SQLITE
          assert (p->dfk == 999);
          assert (*p->ac3 == 1);
          assert (!p->acn);
          assert (*p->acnn == 1);
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
