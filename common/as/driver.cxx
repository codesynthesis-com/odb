// file      : common/as/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test C++ type mapping (#pragma map type as ...).
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Test basic type mapping functionality.
    //
    {
      using namespace test1;

      object o1 (true, green, 123, 234);
      o1.m[false] = 123;
      o1.v.push_back (o1.ip);
      o1.cv.push_back (red);
      o1.cv.push_back (green);

      object o2 (false, blue, 234, 456);
      o2.m[true] = 234;
      o2.v.push_back (o2.ip);
      o2.cv.push_back (green);
      o2.cv.push_back (blue);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.b = false;
      o1.c = blue;
      o1.ip.first++;
      o1.ip.second--;
      o1.m[false]++;
      o1.m[true] = 234;
      o1.v.back () = o1.ip;
      o1.cv.modify_front () = green;
      o1.cv.push_back (red);

      o2.b = true;
      o2.c = red;
      o2.ip.first--;
      o2.ip.second++;
      o2.m[true]--;
      o2.m[false] = 345;
      o2.v.push_back (o2.ip);
      o2.cv.pop_back ();

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test wrapped simple type mapping.
    //
    {
      using namespace test2;

      object o1;
      o1.v.push_back (null_bool ());
      o1.v.push_back (false);

      object o2;
      o2.b = true;
      o2.v.push_back (true);
      o2.v.push_back (null_bool ());

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.b = false;
      o1.v[0] = true;
      o1.v[1].reset ();

      o2.b.reset ();
      o2.v.push_back (false);

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test wrapped composite type mapping.
    //
    {
      using namespace test3;

      object o1;
      o1.ip = intp (0, 0); // NULL
      o1.npv.push_back (o1.np);
      o1.ipv.push_back (o1.ip);

      object o2;
      o2.np = intp (123, 234);
      o1.ip = intp (234, 123);
      o2.npv.push_back (o2.np);
      o2.ipv.push_back (o2.ip);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.np = o1.npv[0] = intp (234, 456);
      o1.ip = o1.ipv.modify_at (0) = intp (456, 234);

      o2.np.reset ();
      o2.npv[0].reset ();
      o2.ip = o2.ipv.modify_at (0) = intp (0, 0); // NULL

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test id type mapping.
    //
    {
      using namespace test4;

      object o1 (123);
      o1.v.push_back (1);
      o1.v.push_back (2);
      o1.v.push_back (3);

      object o2 (234);
      o2.v.push_back (3);
      o2.v.push_back (2);
      o2.v.push_back (1);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.i++;
      o1.v.pop_back ();
      o1.v.modify_front ()++;

      o2.i--;
      o2.v.clear ();
      o2.v.push_back (4);

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test version type mapping.
    //
    {
      using namespace test5;

      object o1 (100, 123);
      object o2 (200, 234);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));

        assert (*p1 == o1);
        assert (*p2 == o2);

        p1->i--;
        p2->i++;

        db->update (*p1);
        db->update (*p2);

        t.commit ();
      }

      {
        transaction t (db->begin ());

        for (;;)
        {
          o1.i++;
          o2.i--;

          try
          {

            db->update (o1);
            db->update (o2);
            break;
          }
          catch (const odb::object_changed&)
          {
            db->reload (o1);
            db->reload (o2);
          }
        }

        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
