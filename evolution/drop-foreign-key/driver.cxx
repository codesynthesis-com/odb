// file      : evolution/drop-foreign-key/driver.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test dropping a foreign key.
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

        object o (1);
        o.o1 = new object (2);
        o.o2 = new object (3);

        {
          transaction t (db->begin ());
          db->persist (o.o1);
          db->persist (o.o2);
          db->persist (o);
          t.commit ();
        }

        // The foreign key constraint is there.
        //
        try
        {
          object o (11);
          o.o1 = new object (12);
          o.o2 = new object (13);

          transaction t (db->begin ());
          db->persist (o);
          assert (false);
        }
        catch (const odb::exception& ) {}
        break;
      }
    case 2:
      {
        using namespace v3;

        // The data is still there but the constraints are gone.
        //
        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> (1));

          assert (p->o1 == 2);
          assert (p->o2 == 3);

          db->erase<object> (p->o1);
          db->erase<object> (p->o2);

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
          assert (p->o1 == 2);
          assert (p->o2 == 3);
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
