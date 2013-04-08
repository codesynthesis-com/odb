// file      : evolution/combined/driver.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Combined schema evolution test.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test1.hxx"
#include "test2.hxx"
#include "test1-odb.hxx"
#include "test2-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

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

        object o ("1");
        o.dui = 1;
        o.dfk = new object1 (1);
        o.acn = 1;
        o.anui = 1;
        o.dnui = 1;
        o.dt.push_back (1);
        o.dc = 1;
        o.acnn.reset ();
        o.afk = 1;
        o.aui = 1;

        {
          transaction t (db->begin ());
          db->persist (o.dfk);
          db->persist (o);
          t.commit ();
        }
        break;
      }
    case 2:
      {
        using namespace v3;

        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> ("1"));

          assert (p->ac1 == 999);
          assert (!p->ac2);
          assert (!p->ac3);

          // Migrate.
          //
          p->dfk = 999;
          p->at.push_back ("abc");
          p->ac3 = 1;
          p->acn.reset ();
          p->acnn = 1;

          db->update (*p);

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
          assert (p->dfk == 999);
          assert (p->at[0] == "abc");
          assert (*p->ac3 == 1);
          assert (!p->acn);
          assert (*p->acnn == 1);

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
