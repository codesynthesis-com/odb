// file      : common/id/nested/driver.cxx
// copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test nested ids.
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

struct failed {};

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));


    // Simple nested id.
    //
    {
      using namespace test1;

      object o1 (1, "a", 3);
      o1.v.push_back (123);

      object o2 (4, "b", 6);
      o2.v.push_back (234);

      object1 o (new object (10, "abc", 11));

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o.p);
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id.y));
        auto_ptr<object> p2 (db->load<object> (o2.id.y));
        auto_ptr<object1> p (db->load<object1> (o.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
        assert (*p == o);
      }

      o1.z++;
      o1.v.pop_back ();
      o1.v.push_back (234);

      o2.z--;
      o2.v.back ()++;
      o2.v.push_back (123);

      delete o.p;
      o.p = new object (20, "xyz", 11);

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        db->persist (o.p);
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id.y));
        auto_ptr<object> p2 (db->load<object> (o2.id.y));
        auto_ptr<object1> p (db->load<object1> (o.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
        assert (*p == o);
      }
    }

    // Composite nested id.
    //
    {
      using namespace test2;

      object o1 (1, 2, "a", 123);
      o1.v.push_back (123);

      object o2 (1, 3, "b", 234);
      o2.v.push_back (234);

      object1 o (new object (2, 2, "abc", 123));
      o.p->v.push_back (345);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o.p);
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id.c));
        auto_ptr<object> p2 (db->load<object> (o2.id.c));
        auto_ptr<object1> p (db->load<object1> (o.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
        assert (*p == o);
      }

      o1.z++;
      o1.v.pop_back ();
      o1.v.push_back (234);

      o2.z--;
      o2.v.modify_back ()++;
      o2.v.push_back (123);

      delete o.p;
      o.p = new object (2, 3, "xyz", 234);

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        db->persist (o.p);
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id.c));
        auto_ptr<object> p2 (db->load<object> (o2.id.c));
        auto_ptr<object1> p (db->load<object1> (o.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
        assert (*p == o);
      }
    }

    // Custom/by-value access.
    //
    {
      using namespace test3;

      object o1 (1, "a", 3);
      object o2 (4, "b", 6);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id.y));
        auto_ptr<object> p2 (db->load<object> (o2.id.y));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.z++;
      o2.z--;

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id.y));
        auto_ptr<object> p2 (db->load<object> (o2.id.y));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Polymorphic.
    //
    {
      using namespace test4;

      base o1 (1, "a");
      object o2 (2, "b", 1);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> p1 (db->load<base> (o1.id.y));
        auto_ptr<object> p2 (db->load<object> (o2.id.y));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o2.z--;

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> p1 (db->load<base> (o1.id.y));
        auto_ptr<object> p2 (db->load<object> (o2.id.y));
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
