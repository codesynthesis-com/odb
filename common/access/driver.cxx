// file      : common/access/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test accessor/modifier expressions.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

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

    // Test basic accessor/modifier functionality.
    //
    {
      using namespace test1;

      object o (1, 623, 723);
      o.i1 () = 123;
      o.i2 (223);
      o.i3 () = 323;
      o.i4 () = 423;
      o.set_i5 (523);
      o.s1 ("1bc");
      memcpy (o.b1 (), "123456789012345", 16);
      o.b2 ("123456789012345");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id ()));
        t.commit ();

        assert (o == *p);
      }
    }

    // Test composite accessor/modifier functionality.
    //
    {
      using namespace test2;

      object o (1);

      o.v1 () = value (1123, 1234);
      o.v2 (value (2123, 2234));
      o.v3_i1 (3123);
      o.v3_i2 (3223);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id ()));
        t.commit ();

        assert (o == *p);
      }
    }

    // Test object pointer accessor/modifier functionality.
    //
    {
      using namespace test3;

      object2 o (1);
      o.p1 ().reset (new object1 (1));
      o.p2 (object1_ptr (new object1 (2)));

      {
        transaction t (db->begin ());
        const object1_ptr& ptr (o.p1 ());
        db->persist (ptr);
        db->persist (o.p2 ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object2> p (db->load<object2> (o.id ()));
        t.commit ();

        assert (p->p1 ()->id () == o.p1 ()->id () &&
                p->p2 ()->id () == o.p2 ()->id ());
      }
    }

    // Test container accessor/modifier functionality.
    //
    {
      using namespace test4;

      object o (1);
      o.c1 ().push_back (1123);
      o.c1 ().push_back (1124);
      o.c1 ().push_back (1125);

      {
        std::vector<int> v;
        v.push_back (2123);
        v.push_back (2124);
        v.push_back (2125);
        o.c2 (v);
      }

      o.v1 ().c1 ().push_back (1123);
      o.v1 ().c1 ().push_back (1124);
      o.v1 ().c1 ().push_back (1125);

      {
        std::vector<int> v;
        v.push_back (2123);
        v.push_back (2124);
        v.push_back (2125);
        o.v1 ().c2 (v);
      }

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id ()));
        t.commit ();

        assert (o == *p);
      }
    }

    // Test id accessor/modifier functionality.
    //
    {
      using namespace test5;

      object1 o1;
      object2 o2;
      object3 o3;
      object4 o4;
      o4.id (uuid ("\x60\x1D\x17\xF0-\x60\x05-\x47\x23-\x95\x37-"
                   "\xC1\xF8\x94\x41\x2B\xEC"));

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        db->persist (o4);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object1> p1 (db->load<object1> (o1.id ()));
        auto_ptr<object2> p2 (db->load<object2> (o2.id ()));
        auto_ptr<object3> p3 (db->load<object3> (o3.id_));
        auto_ptr<object4> p4 (db->load<object4> (o4.id ()));
        t.commit ();
      }
    }

    // Test version accessor/modifier functionality.
    //
    {
      using namespace test6;

      object1 o1 (1);
      object2 o2;
      object3 o3 (1);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();

        assert (o1.version () == 1);
        assert (o2.version () == 1);
        assert (o3.version_ == 1);
      }

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        db->update (o3);
        t.commit ();

        assert (o1.version () == 2);
        assert (o2.version () == 2);
        assert (o3.version_ == 2);
      }
    }

    // Test basic accessor/modifier functionality.
    //
    {
      using namespace test7;

      object o (1);
      o.i1 () = 123;
      o.set_i2 (223);
      o.setI3 (323);
      o.seti4 (423);
      o.i5 () = 523;
      o.i6 () = 623;
      o.SetI7 (723);
      memcpy (o.b1 (), "123456789012345", 16);
      o.b2 ("123456789012345");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id_));
        t.commit ();

        assert (o == *p);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
