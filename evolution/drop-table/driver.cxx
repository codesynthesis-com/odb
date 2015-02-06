// file      : evolution/drop-table/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test dropping a table (object, container).
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
        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::drop_schema (*db);
          schema_catalog::create_schema (*db, "", false);
          schema_catalog::migrate_schema (*db, 2);
          t.commit ();
        }

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
        }

        // Polymorphism test.
        //
        {
          // We have to use v3 here because the discriminator includes
          // the namespace.
          //
          using namespace v3;

          base b (123, "abc");
          derived d1 (234, "bcd");
          derived d2 (345, "cde");

          {
            transaction t (db->begin ());
            db->persist (b);
            db->persist (d1);
            db->persist (d2);
            t.commit ();
          }
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

        // Only object is still there.
        //
        {
          transaction t (db->begin ());
          auto_ptr<object> p (db->load<object> (1));
          assert (p->str == "abc");
          t.commit ();
        }

        // Polymorphism test.
        //
        {
          transaction t (db->begin ());
          assert (size (db->query<root> ()) == 1);
          assert (size (db->query<base> ()) == 1);
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
