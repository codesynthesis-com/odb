// file      : common/view/olv/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test object loading views.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>
#include <typeinfo>

#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test1.hxx"
#include "test2.hxx"
#include "test3.hxx"
#include "test4.hxx"
#include "test5.hxx"
#include "test6.hxx"
#include "test7.hxx"
#include "test8.hxx"
#include "test9.hxx"

#include "test1-odb.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"
#include "test4-odb.hxx"
#include "test5-odb.hxx"
#include "test6-odb.hxx"
#include "test7-odb.hxx"
#include "test8-odb.hxx"
#include "test9-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Test basic object loading functionality.
    //
    {
      using namespace test1;

      {
        object1 o1a (1, 123);
        object2 o2 (1, "abc");

        transaction t (db->begin ());
        db->persist (o1a);
        db->persist (o2);
        t.commit ();
      }

      {
        typedef odb::query<view1> query;

        transaction t (db->begin ());
        view1 v (db->query_value<view1> (query::object1::n == 123));
        assert (v.o2->s == "abc");
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view2 v (db->query_value<view2> ());
        assert (v.o1->n == 123 && v.o2->s == "abc");
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view3 v (db->query_value<view3> ());
        assert (v.o1->n == 123 && v.o2->s == "abc");
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view4 v (db->query_value<view4> ());
        assert (v.s == "abc" && v.o2->s == "abc" && v.id == 1 &&
                v.o1->n == 123 && v.n == 123);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view4 v (db->query_value<view4> ());
        assert (v.s == "abc" && v.o2->s == "abc" && v.id == 1 &&
                v.o1->n == 123 && v.n == 123);
        t.commit ();
      }

      {
        typedef odb::query<view5> query;

        object1 o1b (123, 1);

        transaction t (db->begin ());
        db->persist (o1b);
        view5 v (db->query_value<view5> (query::o1b::n == 1));
        assert (v.o1a->n == 123 && v.o2->s == "abc" && v.o1b->n == 1);
        t.commit ();
      }
    }

    // Test loading of object pointers inside objects.
    //
    {
      using namespace test2;

      shared_ptr<object1> o1 (new object1 (123));
      shared_ptr<object2> o2 (new object2 ("abc", o1));

      {

        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1 v (db->query_value<view1> ());
        assert (v.o2->s == "abc" && v.o2->o1->n == 123);
        t.commit ();
      }

      {
        // Check session interaction.
        //
        transaction t (db->begin ());
        session s;
        shared_ptr<object2> o2a (db->load<object2> (o2->id));
        view1 v (db->query_value<view1> ());
        assert (v.o2 == o2a);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        session s;
        view2 v (db->query_value<view2> ());
        assert (v.o1->n == 123 && v.o2->s == "abc" && v.o2->o1 == v.o1);
        t.commit ();
      }

      shared_ptr<object3> o3 (new object3 (o2));

      {
        transaction t (db->begin ());
        db->persist (o3);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        session s;
        view3 v (db->query_value<view3> ());
        assert (v.o1->n == 123 && v.o3->o2->s == "abc" &&
                v.o3->o2->o1 == v.o1);
        t.commit ();
      }

      shared_ptr<object1> o1b (new object1 (234));
      shared_ptr<object2> o2b (new object2 ("bcd", o1b));
      shared_ptr<object4> o4 (new object4);
      o4->o2.push_back (o2);
      o4->o2.push_back (o2b);

      {
        transaction t (db->begin ());
        db->persist (o1b);
        db->persist (o2b);
        db->persist (o4);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view4 v (db->query_value<view4> ());
        assert (v.o4->o2[0]->s == "abc" && v.o4->o2[0]->o1->n == 123 &&
                v.o4->o2[1]->s == "bcd" && v.o4->o2[1]->o1->n == 234);
        t.commit ();
      }

      {
        typedef odb::query<view5> query;
        typedef odb::result<view5> result;

        transaction t (db->begin ());
        session s;
        result r (db->query<view5> ("ORDER BY" + query::object1::id));
        result::iterator i (r.begin ());

        assert (i != r.end ());
        {
          const view5& v (*i);

          assert (v.o4->o2[0]->s == "abc" && v.o4->o2[0]->o1->n == 123 &&
                  v.o4->o2[1]->s == "bcd" && v.o4->o2[1]->o1->n == 234 &&
                  v.o4->o2[0]->o1 == v.o1);
        }
        assert (++i != r.end ());
        {
          const view5& v (*i);

          assert (v.o4->o2[0]->s == "abc" && v.o4->o2[0]->o1->n == 123 &&
                  v.o4->o2[1]->s == "bcd" && v.o4->o2[1]->o1->n == 234 &&
                  v.o4->o2[1]->o1 == v.o1);
        }
        assert (++i == r.end ());
        t.commit ();
      }

      shared_ptr<object5> o5 (new object5 (o1b, o2));

      {
        transaction t (db->begin ());
        db->persist (o5);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view6 v (db->query_value<view6> ());
        assert (v.o1a->n == 123 && v.o1b->n == 234);
        t.commit ();
      }
    }

    // Test JOINs for pointed-to objects, existing and automatically added.
    //
    {
      using namespace test3;

      shared_ptr<object1> o1 (new object1 (123));
      shared_ptr<object2> o2 (new object2 ("abc"));

      o1->o2 = o2;
      o2->o1 = o1;

      {

        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1a v (db->query_value<view1a> ());
        // VC11
        assert (v.o1->n == 123 && v.o1->o2.object_id<object2> () == o2->id);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1b v (db->query_value<view1b> ());
        // VC11
        assert (v.o1->n == 123 && v.o1->o2.object_id<object2> () == o2->id);
        t.commit ();
      }

      // Container case.
      //

      shared_ptr<object3> o3 (new object3 (123));
      shared_ptr<object4> o4 (new object4 ("abc"));

      o3->o4 = o4;
      o4->o3.push_back (o3);

      {

        transaction t (db->begin ());
        db->persist (o3);
        db->persist (o4);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view2a v (db->query_value<view2a> ());
        // VC11
        assert (v.o3->n == 123 && v.o3->o4.object_id<object4> () == o4->id);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view2b v (db->query_value<view2b> ());
        // VC11
        assert (v.o3->n == 123 && v.o3->o4.object_id<object4> () == o4->id);
        t.commit ();
      }
    }

    // Test by-value load.
    //
    {
      using namespace test4;

      {
        object1 o1 (1, 123);
        object2 o2 (1, "abc", &o1);

        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1 v (db->query_value<view1> ());
        assert (v.o1.n == 123);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1a v (db->query_value<view1a> ());
        assert (!v.o1_null && v.o1.n == 123);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1b v (db->query_value<view1b> ());
        assert (/*v.o1_p == &v.o1 && */ v.o1.n == 123); // Copy ctor.
        t.commit ();
      }

      {
        typedef odb::result<view1c> result;

        transaction t (db->begin ());
        result r (db->query<view1c> ());
        result::iterator i (r.begin ());
        assert (i != r.end ());

        object1 o1;
        view1c v (o1);
        i.load (v);

        assert (v.o1_p == &o1 && o1.n == 123);

        assert (++i == r.end ());
        t.commit ();
      }

      {
        transaction t (db->begin ());
        session s;
        view2 v (db->query_value<view2> ());
        assert (v.o1.n == 123 && v.o2.s == "abc" && v.o2.o1 == &v.o1);
        t.commit ();
      }

      object1 o1b (2, 234);

      {
        transaction t (db->begin ());
        db->persist (o1b);
        t.commit ();
      }

      {
        typedef odb::query<view2a> query;

        transaction t (db->begin ());
        session s;
        view2a v (db->query_value<view2a> (query::object1::id == 2));
        assert (v.o1.n == 234 && v.o2_null);
        t.commit ();
      }

      shared_ptr<object3> o3 (new object3 (1, 123));

      {
        transaction t (db->begin ());
        db->persist (o3);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        {
          view3 v (db->query_value<view3> ());
          assert (v.o3_p == &v.o3 && v.o3.n == 123); // Load into value.
        }
        session s; // Load into cache.
        shared_ptr<object3> o3a (db->load<object3> (o3->id));
        {
          view3 v (db->query_value<view3> ());
          assert (v.o3_p == o3a.get ()); // Load from cache.
        }
        t.commit ();
      }
    }

    // Test NULL object pointers.
    //
    {
      using namespace test5;

      shared_ptr<object1> o1a (new object1 (123));
      shared_ptr<object1> o1b (new object1 (234));
      shared_ptr<object2> o2 (new object2 ("abc", o1a));

      {
        transaction t (db->begin ());
        db->persist (o1a);
        db->persist (o1b);
        db->persist (o2);
        t.commit ();
      }

      {
        typedef odb::query<view1> query;
        typedef odb::result<view1> result;

        transaction t (db->begin ());
        session s;
        result r (db->query<view1> ("ORDER BY" + query::object1::id));
        result::iterator i (r.begin ());

        assert (i != r.end ());
        {
          const view1& v (*i);
          assert (v.o1->n == 123 && v.o2->s == "abc" && v.o2->o1 == v.o1);
        }
        assert (++i != r.end ());
        {
          const view1& v (*i);
          assert (v.o1->n == 234 && !v.o2);
        }
        assert (++i == r.end ());
        t.commit ();
      }

      shared_ptr<object3> o3a (new object3 (make_pair (1, 1), 123));
      shared_ptr<object3> o3b (new object3 (make_pair (2, 2), 234));
      shared_ptr<object4> o4 (new object4 ("abc", o3a));

      {
        transaction t (db->begin ());
        db->persist (o3a);
        db->persist (o3b);
        db->persist (o4);
        t.commit ();
      }

      {
        typedef odb::query<view2> query;
        typedef odb::result<view2> result;

        transaction t (db->begin ());
        session s;
        result r (db->query<view2> ("ORDER BY" + query::object3::n));
        result::iterator i (r.begin ());

        assert (i != r.end ());
        {
          const view2& v (*i);
          assert (v.o3->n == 123 && v.o4->s == "abc" && v.o4->o3 == v.o3);
        }
        assert (++i != r.end ());
        {
          const view2& v (*i);
          assert (v.o3->n == 234 && !v.o4);
        }
        assert (++i == r.end ());
        t.commit ();
      }
    }

    // Test interaction with sections.
    //
    {
      using namespace test6;

      shared_ptr<object1> o1 (new object1 (123));
      shared_ptr<object2> o2 (new object2 ("abc", o1));

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        view1 v (db->query_value<view1> ());
        assert (v.o1->n == 123 && v.o2->s == "abc" &&
                !v.o2->r.loaded () && !v.o2->o1);
        t.commit ();
      }
    }

    // Test explicit conversion to smart pointer member.
    //
    {
      using namespace test7;

      object1 o1 (123);
      object2 o2 ("abc", &o1);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        session s;
        view1 v (db->query_value<view1> ());
        assert (v.o1->n == 123 && v.o2->s == "abc" && v.o2->o1 == v.o1.get ());
        t.commit ();
      }
    }

    // Test loading objects without id.
    //
    {
      using namespace test8;

      object1 o1 (123);
      object2 o2 ("abc", &o1);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        session s;
        view1 v (db->query_value<view1> ());
        assert (v.o1->n == 123 && v.o2->s == "abc" && v.o2->o1 == v.o1.get ());
        t.commit ();
      }
    }

    // Test loading polymorphic objects.
    //
    {
      using namespace test9;

      root r (1);
      base b (2, "a");
      derived d (3, "b", true);

      {
        transaction t (db->begin ());
        db->persist (r);
        db->persist (b);
        db->persist (d);
        t.commit ();
      }

      {
        transaction t (db->begin ());

        // Load via root.
        //
        {
          view1r r (db->query_value<view1r> (query<view1r>::n == 1));
          assert (r.n == 1 && r.o->n == 1 && typeid (*r.o) == typeid (root));
        }

        {
          view1r r (db->query_value<view1r> (query<view1r>::n == 2));
          assert (r.n == 2 && r.o->n == 2 && typeid (*r.o) == typeid (base));
          base& b (dynamic_cast<base&> (*r.o));
          assert (b.s == "a");
        }

        {
          view1r r (db->query_value<view1r> (query<view1r>::n == 3));
          assert (r.n == 3 && r.o->n == 3 && typeid (*r.o) == typeid (derived));
          derived& d (dynamic_cast<derived&> (*r.o));
          assert (d.s == "b" && d.b);
        }

        // Load via base.
        //
        {
          view1b r (db->query_value<view1b> (query<view1b>::n == 2));
          assert (r.s == "a" && r.n == 2 && r.o->n == 2 && b.s == "a");
        }

        {
          view1b r (db->query_value<view1b> (query<view1b>::n == 3));
          assert (r.s == "b" && r.n == 3 && r.o->n == 3 &&
                  typeid (*r.o) == typeid (derived));
          derived& d (dynamic_cast<derived&> (*r.o));
          assert (d.s == "b" && d.b);
        }

        // Load via derived.
        //
        {
          view1d r (db->query_value<view1d> ());
          assert (r.s == "b" && r.n == 3 &&
                  r.o->n == 3 && r.o->s == "b" && r.o->b);
        }

        t.commit ();
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
