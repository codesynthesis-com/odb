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

      object o1 (true, 123, 234);
      o1.m[false] = 123;
      o1.v.push_back (o1.ip);

      object o2 (false, 234, 456);
      o2.m[true] = 234;
      o2.v.push_back (o2.ip);

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
      o1.ip.first++;
      o1.ip.second--;
      o1.m[false]++;
      o1.m[true] = 234;
      o1.v.back () = o1.ip;

      o2.b = true;
      o2.ip.first--;
      o2.ip.second++;
      o2.m[true]--;
      o2.m[false] = 345;
      o2.v.push_back (o2.ip);

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
      object o2;
      o2.b = true;

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

        assert (p1->b == o1.b);
        assert (p2->b == o2.b);
      }

      o1.b = false;
      o2.b.reset ();

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

        assert (p1->b == o1.b);
        assert (p2->b == o2.b);
      }
    }

    // Test wrapped composite type mapping.
    //
    {
      using namespace test3;

      object o1;
      o1.ip = intp (0, 0); // NULL

      object o2;
      o2.np = intp (123, 234);
      o1.ip = intp (234, 123);

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

        assert (p1->np == o1.np && p1->ip == o1.ip);
        assert (p2->np == o2.np && p2->ip == o2.ip);
      }

      o1.np = intp (234, 456);
      o1.ip = intp (456, 234);

      o2.np.reset ();
      o2.ip = intp (0, 0); // NULL

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

        assert (p1->np == o1.np && p1->ip == o1.ip);
        assert (p2->np == o2.np && p2->ip == o2.ip);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
