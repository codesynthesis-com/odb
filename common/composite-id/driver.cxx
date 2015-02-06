// file      : common/composite-id/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test composite object ids.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
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

    // Test 1.
    //
    {
      using namespace test1;

      object o1 (scomp ("aaa", "bbb", "ccc"), 123);
      o1.vec.push_back (scomp ("xxx", "xxx", "xxx"));
      o1.vec.push_back (scomp ("yyy", "yyy", "yyy"));

      object o2 (scomp ("aaa", "bbb", "ccd"), 234);
      o2.vec.push_back (scomp ("zzz", "", "zzz"));

      object o3 (scomp ("baa", "bbb", "ccc"), 345);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        auto_ptr<object> p3 (db->load<object> (o3.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
        assert (*p3 == o3);
      }

      // Update.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o1.id));
        p->num++;
        db->update (*p);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o1.id));
        t.commit ();

        assert (p->num == o1.num + 1);
      }

      // Erase.
      //
      {
        transaction t (db->begin ());
        db->erase<object> (o1.id);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->find<object> (o1.id));
        assert (p.get () == 0);
        t.commit ();
      }
    }

    // Test 2.
    //
    {
      using namespace test2;

      object2 o2 (ncomp (2, 0, 1));
      o2.o1 = new object1 (scomp ("o1", "o2", "aaa"));

      object3 o3 (ncomp (3, 0, 1));
      o3.o1.push_back (new object1 (scomp ("o1", "o3", "aaa")));
      o3.o1.push_back (new object1 (scomp ("o1", "o3", "bbb")));

      object4 o4 (ncomp (4, 0, 1));
      o4.c.o2 = new object2 (ncomp (2, 4, 1));
      o4.c.o2->o1 = new object1 (scomp ("o1", "o2", "ccc"));

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o2.o1);
        db->persist (o2);
        db->persist (o3.o1[0]);
        db->persist (o3.o1[1]);
        db->persist (o3);
        db->persist (o4.c.o2->o1);
        db->persist (o4.c.o2);
        db->persist (o4);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object2> p2 (db->load<object2> (o2.id));
        auto_ptr<object3> p3 (db->load<object3> (o3.id));
        auto_ptr<object4> p4 (db->load<object4> (o4.id));
        t.commit ();

        assert (p2->o1->id == o2.o1->id);
        assert (p3->o1.size () == o3.o1.size ());
        assert (p3->o1[0]->id == o3.o1[0]->id);
        assert (p3->o1[1]->id == o3.o1[1]->id);
        assert (p4->c.o2->id == o4.c.o2->id);
        assert (p4->c.o2->o1->id == o4.c.o2->o1->id);
      }

      // Update.
      //
      {
        scomp id2, id3;

        {
          transaction t (db->begin ());

          auto_ptr<object2> p2 (db->load<object2> (o2.id));
          delete p2->o1;
          p2->o1 = new object1 (scomp ("o1", "o2", "bbb"));
          id2 = db->persist (p2->o1);
          db->update (*p2);

          auto_ptr<object3> p3 (db->load<object3> (o3.id));
          delete p3->o1.back ();
          p3->o1.pop_back ();
          p3->o1.push_back (new object1 (scomp ("o1", "o3", "ccc")));
          id3 = db->persist (p3->o1.back ());
          db->update (*p3);

          t.commit ();
        }

        {
          transaction t (db->begin ());
          auto_ptr<object2> p2 (db->load<object2> (o2.id));
          auto_ptr<object3> p3 (db->load<object3> (o3.id));
          t.commit ();

          assert (p2->o1->id == id2);
          assert (p3->o1.back ()->id == id3);
        }
      }

      // Query.
      //
      {
        {
          typedef odb::query<object2> query;
          typedef odb::result<object2> result;

          transaction t (db->begin ());

          {
            result r (db->query<object2> (query::o1->id.str3 == "bbb"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            assert (i->id == o2.id);
            assert (++i == r.end ());
          }

          {
            // As id (dual interface).
            //
            result r (db->query<object2> (query::o1.str3 == "bbb"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            assert (i->id == o2.id);
            assert (++i == r.end ());
          }

          t.commit ();
        }

        // Second level composite object pointer.
        //
        {
          typedef odb::query<object4> query;
          typedef odb::result<object4> result;

          transaction t (db->begin ());

          result r (db->query<object4> (query::c.o2->o1.str3 == "ccc"));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->id == o4.id);
          assert (++i == r.end ());

          t.commit ();
        }
      }

      // View.
      //
      {
        transaction t (db->begin ());

        {
          typedef odb::query<view2> query;
          typedef odb::result<view2> result;

          result r (db->query<view2> (query::object2::id.num2 == 0));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->num == 1 && i->str == "bbb");
          assert (++i == r.end ());
        }

        {
          typedef odb::query<view3> query;
          typedef odb::result<view3> result;

          result r (db->query<view3> ((query::object3::id.num2 == 0) +
                                      "ORDER BY" + query::object1::id.str3));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->num == 1 && i->str == "aaa");
          assert (++i != r.end ());
          assert (i->num == 1 && i->str == "ccc");
          assert (++i == r.end ());
        }

        {
          typedef odb::query<view4> query;
          typedef odb::result<view4> result;

          result r (db->query<view4> (query::object4::id.num2 == 0));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->num4 == 1 && i->num2 == 1 && i->str == "ccc");
          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // Test 3.
    //
    {
      using namespace test3;

      object2 o2 (ncomp (2, 0, 1));
      o2.o1 = new object1 (scomp ("o1", "o2", "aaa"));
      o2.o1->o2 = &o2;

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o2.o1);
        db->persist (o2);
        t.commit ();
      }

      // Load.
      //
      {
        session s;
        transaction t (db->begin ());
        auto_ptr<object2> p2 (db->load<object2> (o2.id));
        t.commit ();

        assert (p2->o1->o2->id == o2.id);
      }

      // Query.
      //
      {
        typedef odb::query<object1> query;
        typedef odb::result<object1> result;

        transaction t (db->begin ());

        {
          session s;

          result r (db->query<object1> (query::o2->id.num2 == 0));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->id == o2.o1->id);

          i->o2->o1 = 0;
          delete i->o2;

          assert (++i == r.end ());
        }

        t.commit ();
      }

      // View.
      //
      {
        typedef odb::query<view> query;
        typedef odb::result<view> result;

        transaction t (db->begin ());

        result r (db->query<view> (query::object1::id.str2 == "o2"));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->num == 1 && i->str == "aaa");
        assert (++i == r.end ());

        t.commit ();
      }
    }

    // Test 4.
    //
    {
      using namespace test4;

      object2 o2 (ncomp (2, 0, 1));

      o2.o1.push_back (new object1 (scomp ("o1", "o2", "aaa")));
      o2.o1.back ()->o2 = &o2;

      o2.o1.push_back (new object1 (scomp ("o1", "o2", "bbb")));
      o2.o1.back ()->o2 = &o2;

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o2.o1[0]);
        db->persist (o2.o1[1]);
        db->persist (o2);
        t.commit ();
      }

      // Load.
      //
      {
        session s;
        transaction t (db->begin ());
        auto_ptr<object2> p2 (db->load<object2> (o2.id));
        t.commit ();

        assert (p2->o1.size () == 2);
        assert (p2->o1[0]->o2->id == o2.id);
        assert (p2->o1[1]->o2->id == o2.id);
      }

      // Query.
      //
      {
        typedef odb::query<object1> query;
        typedef odb::result<object1> result;

        transaction t (db->begin ());

        {
          session s;

          result r (db->query<object1> (query::o2->id.num2 == 0));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->id == o2.o1[0]->id);
          i->o2->o1.clear ();

          assert (++i != r.end ());
          assert (i->id == o2.o1[1]->id);

          i->o2->o1.clear ();
          delete i->o2;

          assert (++i == r.end ());
        }

        t.commit ();
      }

      // View.
      //
      {
        typedef odb::query<view> query;
        typedef odb::result<view> result;

        transaction t (db->begin ());

        result r (db->query<view> (query::object1::id.str3 == "bbb"));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->num == 1 && i->str == "bbb");
        assert (++i == r.end ());

        t.commit ();
      }
    }

    // Test 5.
    //
    {
      using namespace test5;

      object2 o2 (ncomp (2, 0, 1));

      o2.o1.push_back (new object1 (scomp ("o1", "o2", "aaa")));
      o2.o1.back ()->o2 = &o2;

      o2.o1.push_back (new object1 (scomp ("o1", "o2", "bbb")));
      o2.o1.back ()->o2 = &o2;

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o2.o1[0]);
        db->persist (o2.o1[1]);
        db->persist (o2);
        t.commit ();
      }

      // Load.
      //
      {
        session s;
        transaction t (db->begin ());
        auto_ptr<object2> p2 (db->load<object2> (o2.id));
        t.commit ();

        assert (p2->o1.size () == 2);

        assert (p2->o1[0]->id == o2.o1[0]->id);
        assert (p2->o1[0]->o2->id == o2.id);

        assert (p2->o1[1]->id == o2.o1[1]->id);
        assert (p2->o1[1]->o2->id == o2.id);
      }

      // View.
      //
      {
        typedef odb::query<view> query;
        typedef odb::result<view> result;

        transaction t (db->begin ());

        result r (db->query<view> ((query::object2::id.num2 == 0) +
                                   "ORDER BY" + query::object1::id.str3));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->num == 1 && i->str == "aaa");
        assert (++i != r.end ());
        assert (i->num == 1 && i->str == "bbb");
        assert (++i == r.end ());

        t.commit ();
      }
    }

    // Test 6.
    //
    {
      using namespace test6;

      object2 o2 (ncomp (2, 0, 1));

      o2.o1.push_back (new object1 (scomp ("o1", "o2", "aaa")));
      o2.o1.back ()->o2.push_back (&o2);

      o2.o1.push_back (new object1 (scomp ("o1", "o2", "bbb")));
      o2.o1.back ()->o2.push_back (&o2);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o2.o1[0]);
        db->persist (o2.o1[1]);
        db->persist (o2);
        t.commit ();
      }

      // Load.
      //
      {
        session s;
        transaction t (db->begin ());
        auto_ptr<object2> p2 (db->load<object2> (o2.id));
        t.commit ();

        assert (p2->o1.size () == 2);

        assert (p2->o1[0]->id == o2.o1[0]->id);
        assert (p2->o1[0]->o2[0]->id == o2.id);

        assert (p2->o1[1]->id == o2.o1[1]->id);
        assert (p2->o1[1]->o2[0]->id == o2.id);
      }

      // View.
      //
      {
        typedef odb::query<view> query;
        typedef odb::result<view> result;

        transaction t (db->begin ());

        result r (db->query<view> ((query::object2::id.num2 == 0) +
                                   "ORDER BY" + query::object1::id.str3));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->num == 1 && i->str == "aaa");
        assert (++i != r.end ());
        assert (i->num == 1 && i->str == "bbb");
        assert (++i == r.end ());

        t.commit ();
      }
    }

    // Test 7.
    //
    {
      using namespace test7;

      object o (scomp ("aaa", "bbb", "ccc"), 123);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        t.commit ();

        assert (*p == o);
      }

      // Update.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        p->num++;
        db->update (*p);

        try
        {
          db->update (o);
          assert (false);
        }
        catch (const object_changed&)
        {
        }

        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        t.commit ();

        assert (p->num == o.num + 1);
      }

      // Erase.
      //
      {
        transaction t (db->begin ());

        try
        {
          db->update (o);
          assert (false);
        }
        catch (const object_changed&)
        {
        }

        t.commit ();
      }
    }

    // Test 8.
    //
    {
      using namespace test8;

      object2 o2a, o2b;
      object3 o3;

      o2b.o1 = new object1 (scomp ("222", "aaa", "bbb"), 123);
      o3.o1.push_back (0);
      o3.o1.push_back (new object1 (scomp ("333", "aaa", "bbb"), 234));

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o2a);
        db->persist (o2b);
        db->persist (o2b.o1);
        db->persist (o3);
        db->persist (o3.o1[1]);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object2> p2a (db->load<object2> (o2a.id));
        auto_ptr<object2> p2b (db->load<object2> (o2b.id));
        auto_ptr<object3> p3 (db->load<object3> (o3.id));
        t.commit ();

        assert (p2a->o1 == 0);
        assert (p2b->o1 != 0 && *p2b->o1 == *o2b.o1);
        assert (p3->o1[0] == 0);
        assert (p3->o1[1] != 0 && *p3->o1[1] == *o3.o1[1]);
      }

      // Update.
      //
      {
        object1* o1 (o3.o1[1]);

        o3.o1.clear ();
        o3.o1.push_back (o2b.o1);
        o3.o1.push_back (0);

        o2a.o1 = o1;
        o2b.o1 = 0;

        transaction t (db->begin ());
        db->update (o2a);
        db->update (o2b);
        db->update (o3);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object2> p2a (db->load<object2> (o2a.id));
        auto_ptr<object2> p2b (db->load<object2> (o2b.id));
        auto_ptr<object3> p3 (db->load<object3> (o3.id));
        t.commit ();

        assert (p2a->o1 != 0 && *p2a->o1 == *o2a.o1);
        assert (p2b->o1 == 0);
        assert (p3->o1[0] != 0 && *p3->o1[0] == *o3.o1[0]);
        assert (p3->o1[1] == 0);
      }
    }

    // Test 9.
    {
      using namespace test9;

      object o (123, "abc");
      o.v.push_back (123);

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        result<object> r (db->query<object> ());
        result<object>::iterator i (r.begin ());
        assert (i != r.end () && o == *i && ++i == r.end ());
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
