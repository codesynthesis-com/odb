// file      : evolution/drop-table/driver.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test dropping a table (object, container).
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

        object1 o1;
        o1.o = new object (1);
        o1.o->str = "abc";
        o1.nums.push_back (1);
        o1.nums.push_back (2);
        o1.nums.push_back (3);

        {
          transaction t (db->begin ());
          db->persist (o1.o);
          db->persist (o1);
          t.commit ();
        }
        break;
      }
    case 2:
      {
        using namespace v2; // @@ v3; soft immediate drop

        // Both object and object1 are still there so we can migrate the data.
        //
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
    case 3:
      {
        using namespace v3;

        // Only object is still there.
        //
        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> (1));
          assert (p->str == "abc");
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
