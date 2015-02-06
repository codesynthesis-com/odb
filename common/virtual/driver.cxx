// file      : common/virtual/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test virtual data members.
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

    // Test basic virtual data member functionality.
    //
    {
      using namespace test1;

      object o;
      o.i (123);
      o.c1.i = 123;
      o.c1.s = "abc";
      o.v1.push_back ("abc");
      o.v1.push_back ("abd");
      o.v1.push_back ("abe");
      o.p1 = new object;

      {
        transaction t (db->begin ());
        db->persist (*o.p1);
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id1.v));
        t.commit ();

        assert (o == *p);
      }
    }

    // Test pragma resolution to virtual data member.
    //
    {
      using namespace test2;

      object1 o1 (1);
      o1.o2 = new object2 (1);
      o1.o2->o1 = &o1;

      {
        transaction t (db->begin ());
        db->persist (*o1.o2);
        o1.n1 = o1.o2->id;
        db->persist (o1);
        t.commit ();
      }

      {
        session s;
        transaction t (db->begin ());
        auto_ptr<object1> p (db->load<object1> (o1.id));
        t.commit ();

        assert (p->o2->id == o1.o2->id);
      }

      {
        typedef odb::query<view1> query;
        typedef odb::result<view1> result;

        transaction t (db->begin ());
        result r (db->query<view1> (query::object2::id == o1.o2->id));
        result::iterator i (r.begin ());
        assert (i != r.end () && i->i == o1.n1);
        assert (++i == r.end ());
        t.commit ();
      }

      {
        typedef odb::query<view2> query;
        typedef odb::result<view2> result;

        transaction t (db->begin ());
        result r (db->query<view2> (query::o2::id == o1.o2->id));
        result::iterator i (r.begin ());
        assert (i != r.end () && i->i == o1.n1);
        assert (++i == r.end ());
        t.commit ();
      }

      {
        typedef odb::result<view3> result;

        transaction t (db->begin ());
        result r (db->query<view3> ());
        result::iterator i (r.begin ());
        assert (i != r.end () && i->i == o1.n1);
        assert (++i == r.end ());
        t.commit ();
      }
    }

    // Use virtual data members to implement multi-member composite object id.
    //
    {
      using namespace test3;

      person o;
      o.first_ = "John";
      o.last_ = "Doe";

      name id;
      {
        transaction t (db->begin ());
        id = db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<person> p (db->load<person> (id));
        t.commit ();

        assert (o.first_ == p->first_ && o.last_ == p->last_);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
