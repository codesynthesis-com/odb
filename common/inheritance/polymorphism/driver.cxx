// file      : common/inheritance/polymorphism/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test polymorphic object inheritance.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
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
#include "test10.hxx"
#include "test11.hxx"
#include "test12.hxx"
#include "test13.hxx"
#include "test14.hxx"
#include "test15.hxx"

#include "test1-odb.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"
#include "test4-odb.hxx"
#include "test5-odb.hxx"
#include "test6-odb.hxx"
#include "test7-odb.hxx"
#include "test8-odb.hxx"
#include "test9-odb.hxx"
#include "test10-odb.hxx"
#include "test11-odb.hxx"
#include "test12-odb.hxx"
#include "test13-odb.hxx"
#include "test14-odb.hxx"
#include "test15-odb.hxx"

using namespace std;
using namespace odb::core;

const char* events[] =
{
  "pre_persist",
  "post_persist",
  "pre_load",
  "post_load",
  "pre_update",
  "post_update",
  "pre_erase",
  "post_erase"
};

namespace test6
{
  void base::
  db_callback (callback_event e, database&)
  {
    cout << "base " << events[e] << " " << id << endl;
  }

  void base::
  db_callback (callback_event e, database&) const
  {
    cout << "base " << events[e] << " " << id << " const" << endl;
  }

  void derived::
  db_callback (callback_event e, database&) const
  {
    cout << "derived " << events[e] << " " << id << " const" << endl;
  }
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Test 1: basic polymorphism functionality.
    //
    {
      using namespace test1;

      root r (1, 1);
      base b (2, 2, "bbb");
      derived d (3, 3, "ddd");

      r.strs.push_back ("a");
      r.strs.push_back ("aa");
      r.strs.push_back ("aaa");

      b.nums.push_back (21);
      b.nums.push_back (22);
      b.nums.push_back (23);
      b.strs.push_back ("b");
      b.strs.push_back ("bb");
      b.strs.push_back ("bbb");

      d.nums.push_back (31);
      d.nums.push_back (32);
      d.nums.push_back (33);
      d.strs.push_back ("d");
      d.strs.push_back ("dd");
      d.strs.push_back ("ddd");

      {
        transaction t (db->begin ());

        // Static persist.
        //
        db->persist (r);
        db->persist (b);

        // Dynamic persist.
        //
        root& r (d);
        db->persist (r);

        t.commit ();
      }

      // Static load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pr (db->load<root> (r.id));
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));
        t.commit ();

        assert (*pr == r);
        assert (*pb == b);
        assert (*pd == d);
      }

      // Dynamic load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pb (db->load<root> (b.id));
        auto_ptr<root> pd1 (db->load<root> (d.id));
        auto_ptr<base> pd2 (db->load<base> (d.id));
        t.commit ();

        assert (*pb == b);
        assert (*pd1 == d);
        assert (*pd2 == d);
      }

      // Invalid load.
      //
      {
        transaction t (db->begin ());

        try
        {
          auto_ptr<base> p (db->load<base> (r.id));
          assert (false);
        }
        catch (const object_not_persistent&) {}

        try
        {
          auto_ptr<derived> p (db->load<derived> (b.id));
          assert (false);
        }
        catch (const object_not_persistent&) {}

        t.commit ();
      }

      // Static load into existing instance.
      //
      {
        transaction t (db->begin ());
        root r1;
        db->load (r.id, r1);
        base b1;
        db->load (b.id, b1);
        derived d1;
        db->load (d.id, d1);
        t.commit ();

        assert (r1 == r);
        assert (b1 == b);
        assert (d1 == d);
      }

      // Dynamic load into existing instance.
      //
      {
        transaction t (db->begin ());
        base b1;
        db->load (b.id, static_cast<root&> (b1));
        derived d1;
        db->load (d.id, static_cast<base&> (d1));
        t.commit ();

        assert (b1 == b);
        assert (d1 == d);
      }

      // Invalid load into existing instance.
      //
      {
        transaction t (db->begin ());

        try
        {
          base b;
          db->load (r.id, static_cast<root&> (b));
          assert (false);
        }
        catch (const object_not_persistent&) {}

        try
        {
          derived d;
          db->load (b.id, static_cast<base&> (d));
          assert (false);
        }
        catch (const object_not_persistent&) {}

        t.commit ();
      }

      // Slicing load.
      //
      {
        transaction t (db->begin ());
        root b1;
        db->load (b.id, b1);
        base d1;
        db->load (d.id, d1);
        t.commit ();

        assert (b1 == static_cast<root> (b));
        assert (d1 == static_cast<base> (d));
      }

      // Static reload.
      //
      {
        transaction t (db->begin ());
        root r1;
        r1.id = r.id;
        db->reload (r1);
        base b1;
        b1.id = b.id;
        db->reload (b1);
        derived d1;
        d1.id = d.id;
        db->reload (d1);
        t.commit ();

        assert (r1 == r);
        assert (b1 == b);
        assert (d1 == d);
      }

      // Dynamic reload.
      //
      {
        transaction t (db->begin ());
        base b1;
        b1.id = b.id;
        db->reload (static_cast<root&> (b1));
        derived d1;
        d1.id = d.id;
        db->reload (static_cast<base&> (d1));
        t.commit ();

        assert (b1 == b);
        assert (d1 == d);
      }

      // Invalid reload.
      //
      {
        transaction t (db->begin ());

        try
        {
          base b;
          b.id = r.id;
          db->reload (static_cast<root&> (b));
          assert (false);
        }
        catch (const object_not_persistent&) {}

        try
        {
          derived d;
          d.id = b.id;
          db->reload (static_cast<base&> (d));
          assert (false);
        }
        catch (const object_not_persistent&) {}

        t.commit ();
      }

      // Slicing reload.
      //
      {
        transaction t (db->begin ());
        root b1;
        b1.id = b.id;
        db->reload (b1);
        base d1;
        d1.id = d.id;
        db->reload (d1);
        t.commit ();

        assert (b1 == static_cast<root> (b));
        assert (d1 == static_cast<base> (d));
      }

      // Query.
      //
      {
        typedef odb::query<root> root_query;
        typedef odb::result<root> root_result;

        typedef odb::query<base> base_query;
        typedef odb::result<base> base_result;

        typedef odb::result<derived> derived_result;

        transaction t (db->begin ());

        // Test loading via root.
        //
        {
          root_result qr (db->query<root> ("ORDER BY" + root_query::id));
          root_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == r);
          assert (++i != e && *i == b);
          assert (++i != e && *i == d);
          assert (++i == e);
        }

        // Test loading via base.
        //
        {
          base_result qr (db->query<base> ("ORDER BY" + base_query::id));
          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == b);
          assert (++i != e && *i == d);
          assert (++i == e);
        }

        // Test loading via derived.
        //
        {
          derived_result qr (db->query<derived> ());
          derived_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == d);
          assert (++i == e);
        }

        // Test loading into an existing instance.
        //
        {
          root_result qr (db->query<root> ());

          unsigned short mask (0);

          for (root_result::iterator i (qr.begin ()); i != qr.end (); ++i)
          {
            string ds (i.discriminator ());

            if (ds == "test1::root")
            {
              root r1;
              i.load (r1);
              assert (r1 == r);
              mask |= 1;
            }
            else if (ds == "test1::base")
            {
              base b1;
              i.load (b1);
              assert (b1 == b);
              mask |= 2;
            }
            else if (ds == "test1::derived")
            {
              derived d1;
              i.load (d1);
              assert (d1 == d);
              mask |= 4;
            }
            else
              assert (false);
          }

          assert (mask == 7);
        }

        // Test query conditions with columns from multiple tables.
        //
        {
          base_result qr (
            db->query<base> (
              base_query::num == 3 && base_query::str == "ddd"));

          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == d);
          assert (++i == e);
        }

        // Test discriminator access.
        //
        {
          base_result qr (db->query<base> (base_query::id == 3));
          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i.discriminator () == "test1::derived");
          assert (++i == e);
        }

        // Test loading of an object from the same hierarchy during
        // query result iteration (tests image copying via change
        // callbacks in some databases).
        //
        {
          base_result qr (db->query<base> ());

          unsigned short mask (0);

          for (base_result::iterator i (qr.begin ()); i != qr.end (); ++i)
          {
            string ds (i.discriminator ());

            if (ds == "test1::base")
            {
              auto_ptr<derived> d1 (db->load<derived> (d.id));
              assert (*d1 == d);
              assert (*i == b);
              mask |= 1;
            }
            else if (ds == "test1::derived")
            {
              auto_ptr<base> b1 (db->load<base> (b.id));
              assert (*b1 == b);
              assert (*i == d);
              mask |= 2;
            }
          }

          assert (mask == 3);
        }

        t.commit ();
      }

      // Views.
      //
      {
        typedef odb::query<root_view> root_query;
        typedef odb::result<root_view> root_result;

        typedef odb::query<base_view> base_query;
        typedef odb::result<base_view> base_result;

        typedef odb::result<derived_view> derived_result;

        transaction t (db->begin ());

        // root
        //
        {
          root_result qr (db->query<root_view> ("ORDER BY" + root_query::id));
          root_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i->typeid_ == "test1::root" && i->num == r.num);
          assert (++i != e && i->typeid_ == "test1::base" && i->num == b.num);
          assert (++i != e && i->typeid_ == "test1::derived" && i->num == d.num);
          assert (++i == e);
        }

        // base
        //
        {
          base_result qr (db->query<base_view> ("ORDER BY" + base_query::id));
          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e &&
                  i->id == b.id && i->num == b.num && i->str == b.str);
          assert (++i != e &&
                  i->id == d.id && i->num == d.num && i->str == d.str);
          assert (++i == e);
        }

        // derived
        //
        {
          derived_result qr (db->query<derived_view> ());
          derived_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e &&
                  i->num == d.num && i->str == d.str &&
                  i->dnum == d.dnum && i->dstr == d.dstr);
          assert (++i == e);
        }

        t.commit ();
      }

      // Update.
      //
      r.num++;
      r.strs.push_back ("aaaa");

      b.num++;
      b.str += "b";
      b.nums.push_back (24);
      b.strs.push_back ("bbbb");

      d.num++;
      d.str += "d";
      d.dnum++;
      d.dstr += "d";
      d.nums.push_back (34);
      d.strs.push_back ("dddd");

      {
        transaction t (db->begin ());

        // Static update.
        //
        db->update (r);
        db->update (b);

        // Dynamic update.
        //
        root& r (d);
        db->update (r);

        t.commit ();
      }

      // Verify update.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pr (db->load<root> (r.id));
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));
        t.commit ();

        assert (*pr == r);
        assert (*pb == b);
        assert (*pd == d);
      }

      // Invalid erase via id.
      //
      {
        transaction t (db->begin ());

        try
        {
          db->erase<base> (r.id);
          assert (false);
        }
        catch (const object_not_persistent&) {}

        try
        {
          db->erase<derived> (b.id);
          assert (false);
        }
        catch (const object_not_persistent&) {}

        t.commit ();
      }

      // Static erase via id.
      //
      {
        transaction t (db->begin ());
        db->erase<root> (r.id);
        db->erase<base> (b.id);
        t.commit ();
      }

      // Dynamic erase via id.
      //
      {
        transaction t (db->begin ());
        db->erase<root> (d.id);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->persist (r);
        db->persist (b);
        db->persist (d);
        t.commit ();
      }

      // Static erase via object.
      //
      {
        transaction t (db->begin ());
        db->erase (r);
        db->erase (b);
        t.commit ();
      }

      // Dynamic erase via object.
      //
      {
        const root& r (d);
        transaction t (db->begin ());
        db->erase (r);
        t.commit ();
      }
    }

    // Test 2: inverse object pointers in polymorhic bases.
    //
    {
      using namespace test2;

      derived d (1, "d", 1);
      root_pointer rp (&d);
      base_pointer bp (&d);

      {
        transaction t (db->begin ());
        db->persist (rp);
        db->persist (bp);

        d.rp.reset (*db, &rp);
        d.bp.reset (*db, &bp);

        db->persist (d);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));
        auto_ptr<base> pb (db->load<base> (d.id));
        auto_ptr<root> pr (db->load<root> (d.id));
        t.commit ();

        assert (pd->rp.object_id<root_pointer> () == rp.id &&
                pd->bp.object_id<base_pointer> () == bp.id);

        derived* p (dynamic_cast<derived*> (pb.get ()));
        assert (p != 0 &&
                p->rp.object_id<root_pointer> () == rp.id &&
                p->bp.object_id<base_pointer> () == bp.id);

        p = dynamic_cast<derived*> (pr.get ());
        assert (p != 0 &&
                p->rp.object_id<root_pointer> () == rp.id &&
                p->bp.object_id<base_pointer> () == bp.id);
      }

      // Query.
      //
      {
        typedef odb::query<base> base_query;
        typedef odb::result<base> base_result;

        transaction t (db->begin ());

        // Test query conditions with columns in pointed-to objects from
        // multiple tables.
        //
        {
          base_result qr (
            db->query<base> (
              base_query::rp->id == rp.id &&
              base_query::bp->id == bp.id));

          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i.discriminator () == "test2::derived");
          assert (++i == e);
        }

        t.commit ();
      }

      // Views.
      //
      {
        typedef odb::result<root_view> root_result;
        typedef odb::result<base_view> base_result;

        transaction t (db->begin ());

        // root
        //
        {
          root_result qr (db->query<root_view> ());
          root_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i->rp_id == rp.id && i->r_id == d.id);
          assert (++i == e);
        }

        // base
        //
        {
          base_result qr (db->query<base_view> ());
          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e &&
                  i->bp_id == bp.id && i->b_id == d.id && i->str == d.str);
          assert (++i == e);
        }

        t.commit ();
      }
    }

    // Test 3: delayed loading.
    //
    {
      using namespace test3;

      base b1 (21, 21);
      base b2 (22, 22);
      base b3 (23, 23);

      derived d1 (31, 31, "d");
      derived d2 (32, 32, "dd");
      derived d3 (33, 33, "ddd");

      b1.rptr = new root (1);
      b2.rptr = new base (2, 2);
      b3.rptr = new derived (3, 3, "b3");

      d1.rptr = new root (4);
      d2.rptr = new base (5, 5);
      d3.rptr = new derived (6, 6, "d3");

      d2.bptr = new base (7, 7);
      d3.bptr = new derived (8, 8, "d3b");

      {
        transaction t (db->begin ());
        db->persist (b1);
        db->persist (b2);
        db->persist (b3);

        db->persist (d1);
        db->persist (d2);
        db->persist (d3);

        db->persist (b1.rptr);
        db->persist (b2.rptr);
        db->persist (b3.rptr);

        db->persist (d1.rptr);
        db->persist (d2.rptr);
        db->persist (d3.rptr);

        db->persist (d2.bptr);
        db->persist (d3.bptr);

        t.commit ();
      }

      {
        transaction t (db->begin ());

        {
          auto_ptr<base> p1 (db->load<base> (b1.id));
          auto_ptr<base> p2 (db->load<base> (b2.id));
          auto_ptr<root> p3 (db->load<root> (b3.id));
          assert (*p1 == b1);
          assert (*p2 == b2);
          assert (*p3 == b3);
        }

        {
          auto_ptr<derived> p1 (db->load<derived> (d1.id));
          auto_ptr<base> p2 (db->load<base> (d2.id));
          auto_ptr<root> p3 (db->load<root> (d3.id));
          assert (*p1 == d1);
          assert (*p2 == d2);
          assert (*p3 == d3);
        }

        t.commit ();
      }

      // Query.
      //
      {
        typedef odb::query<derived> derived_query;
        typedef odb::result<derived> derived_result;

        transaction t (db->begin ());

        // Test query conditions with columns in pointed-to objects from
        // multiple tables.
        //
        {
          derived_result qr (
            db->query<derived> (
              derived_query::rptr->id == 6 &&
              derived_query::bptr->id == 8 &&
              derived_query::bptr->num == 8));

          derived_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == d3);
          assert (++i == e);
        }

        t.commit ();
      }

      // Views.
      //
      {
        typedef odb::query<base_view> base_query;
        typedef odb::result<base_view> base_result;

        typedef odb::query<derived_view> derived_query;
        typedef odb::result<derived_view> derived_result;

        typedef odb::query<root_view> root_query;
        typedef odb::result<root_view> root_result;

        transaction t (db->begin ());

        // base
        //
        {
          base_result qr (
            db->query<base_view> (
              base_query::base::num == b2.num &&
              base_query::base::id == b2.id &&
              base_query::r::id == b2.rptr->id));

          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e &&
                  i->b_id == b2.id &&
                  i->r_id == b2.rptr->id &&
                  i->num == b2.num);
          assert (++i == e);
        }

        // derived
        //
        {
          derived_result qr (
            db->query<derived_view> (
              derived_query::d::str == d3.str &&
              derived_query::d::num == d3.num &&
              derived_query::b::num == d3.bptr->num &&
              derived_query::d::id == d3.id &&
              derived_query::b::id == d3.bptr->id &&
              derived_query::r::id == d3.rptr->id));

          derived_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e &&
                  i->d_id == d3.id &&
                  i->b_id == d3.bptr->id &&
                  i->r_id == d3.rptr->id &&
                  i->d_num == d3.num &&
                  i->b_num == d3.bptr->num &&
                  i->str == d3.str);
          assert (++i == e);
        }

        // root
        //
        {
          root_result qr (
            db->query<root_view> (
              root_query::r::id.in (b2.rptr->id, d2.rptr->id)));

          root_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e &&
                  i->r_id == d2.rptr->id &&
                  i->d_id == d2.id &&
                  i->str == d2.str);
          assert (++i == e);
        }

        t.commit ();
      }
    }

    // Test 4: views.
    //
    {
      using namespace test4;

      base1 b1 (21, 1);

      root2 r2 (11, 0);
      base2 b2 (21, 1, "abc");

      {
        transaction t (db->begin ());
        db->persist (b1);
        db->persist (r2);
        db->persist (b2);
        t.commit ();
      }

      {
        typedef odb::query<view1> query;
        typedef odb::result<view1> result;

        transaction t (db->begin ());

        {
          result qr (
            db->query<view1> (
              query::base1::num == b1.num));

          result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i->str == "abc");
          assert (++i == e);
        }

        t.commit ();
      }

      {
        typedef odb::result<view2> result;

        transaction t (db->begin ());

        {
          result qr (db->query<view2> ());
          result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i->min_num == 1);
          assert (++i == e);
        }

        t.commit ();
      }

      {
        typedef odb::result<view3> result;

        transaction t (db->begin ());

        {
          result qr (db->query<view3> ());
          result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && i->str == "abc");
          assert (++i == e);
        }

        t.commit ();
      }
    }

    // Test 5: polymorphism and optimistic concurrency.
    //
    {
      using namespace test5;

      root r (1, 1);
      base b (2, 2, "bbb");
      derived d (3, 3, "ddd");

      r.strs.push_back ("a");
      r.strs.push_back ("aa");
      r.strs.push_back ("aaa");

      b.nums.push_back (21);
      b.nums.push_back (22);
      b.nums.push_back (23);
      b.strs.push_back ("b");
      b.strs.push_back ("bb");
      b.strs.push_back ("bbb");

      d.nums.push_back (31);
      d.nums.push_back (32);
      d.nums.push_back (33);
      d.strs.push_back ("d");
      d.strs.push_back ("dd");
      d.strs.push_back ("ddd");

      {
        transaction t (db->begin ());
        db->persist (r);
        db->persist (b);
        db->persist (d);
        t.commit ();
      }

      // Update.
      //
      {
        transaction t (db->begin ());

        // Root.
        //
        {
          auto_ptr<root> p (db->load<root> (r.id));

          r.num++;
          r.strs.push_back ("aaaa");
          db->update (r);

          p->num--;
          p->strs.pop_back ();
          try
          {
            db->update (p);
            assert (false);
          }
          catch (const odb::object_changed&) {}

          // Make sure the object is intact.
          //
          db->reload (p);
          assert (r == *p);
        }

        // Base.
        //
        {
          auto_ptr<base> p (db->load<base> (b.id));

          b.num++;
          b.str += "b";
          b.strs.push_back ("bbbb");
          b.nums.push_back (24);
          db->update (b);

          p->num--;
          p->str += "B";
          p->strs.pop_back ();
          p->nums.pop_back ();
          try
          {
            db->update (p);
            assert (false);
          }
          catch (const odb::object_changed&) {}

          // Make sure the object is intact.
          //
          db->reload (p);
          assert (b == *p);
        }

        // Derived.
        //
        {
          auto_ptr<root> p (db->load<root> (d.id)); // Via root.

          d.num++;
          d.str += "d";
          d.strs.push_back ("dddd");
          d.nums.push_back (24);
          d.dnum++;
          d.dstr += "d";
          db->update (d);

          derived& d1 (static_cast<derived&> (*p));
          d1.num--;
          d1.str += "D";
          d1.strs.pop_back ();
          d1.nums.pop_back ();
          d1.dnum--;
          d1.dstr += "D";
          try
          {
            db->update (p);
            assert (false);
          }
          catch (const odb::object_changed&) {}

          // Make sure the object is intact.
          //
          db->reload (p);
          assert (d == *p);
        }

        t.commit ();
      }

      // Reload.
      //
      {
        transaction t (db->begin ());

        // Make sure reload doesn't modify the object if the versions
        // match.
        //
        derived d1 (d);
        d1.num++;
        d1.str += "d";
        d1.strs.push_back ("dddd");
        d1.nums.push_back (24);
        d1.dnum++;
        d1.dstr += "d";
        derived d2 (d1);

        db->reload (d1);
        assert (d1 == d2);

        t.commit ();
      }

      // Erase.
      //
      {
        transaction t (db->begin ());

        // Root.
        //
        {
          auto_ptr<root> p (db->load<root> (r.id));

          r.num++;
          r.strs.push_back ("aaaaa");
          db->update (r);

          try
          {
            db->erase (p);
            assert (false);
          }
          catch (const odb::object_changed&) {}

          db->reload (p);
          db->erase (p);
        }

        // Base.
        //
        {
          auto_ptr<base> p (db->load<base> (b.id));

          b.num++;
          b.str += "b";
          b.strs.push_back ("bbbb");
          b.nums.push_back (24);
          db->update (b);

          try
          {
            db->erase (p);
            assert (false);
          }
          catch (const odb::object_changed&) {}

          db->reload (p);
          db->erase (p);
        }

        // Derived.
        //
        {
          auto_ptr<root> p (db->load<root> (d.id)); // Via root.

          d.num++;
          d.str += "d";
          d.strs.push_back ("dddd");
          d.nums.push_back (24);
          d.dnum++;
          d.dstr += "d";
          db->update (d);

          try
          {
            db->erase (p);
            assert (false);
          }
          catch (const odb::object_changed&) {}

          db->reload (p);
          db->erase (p);
        }

        // Try to update non-existent object.
        //
        {
          try
          {
            db->update (d);
            assert (false);
          }
          catch (const odb::object_changed&) {}
        }

        // Try to erase non-existent object.
        //
        {
          try
          {
            db->erase (d);
            assert (false);
          }
          catch (const odb::object_changed&) {}
        }

        t.commit ();
      }
    }

    // Test 6: polymorphism and callbacks.
    //
    {
      using namespace test6;

      base b (1, 1, "bbb");
      auto_ptr<base> d (new derived (2, 2, "ddd"));

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (d);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());

        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<root> pd (db->load<root> (d->id));

        db->load (b.id, *pb);
        db->load (d->id, *pd);

        db->reload (*pb);
        db->reload (*pd);

        t.commit ();
      }

      // Update.
      //
      {
        b.num++;
        d->num++;

        transaction t (db->begin ());
        db->update (b);
        db->update (d);
        t.commit ();
      }

      // Query.
      //
      {
        typedef odb::query<base> query;
        typedef odb::result<base> result;

        transaction t (db->begin ());

        result r (db->query<base> ("ORDER BY" + query::id));
        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          *i;

        t.commit ();
      }

      // Erase.
      //
      {
        transaction t (db->begin ());
        db->erase (b);
        db->erase (d);
        t.commit ();
      }

      // Recursive (delayed) loading.
      //
      {
        derived d (3, 3, "dddd");
        d.ptr.reset (new derived (4, 4, "ddddd"));

        {
          transaction t (db->begin ());
          db->persist (d);
          db->persist (d.ptr);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          auto_ptr<root> p (db->load<root> (d.id));
          t.commit ();
        }
      }
    }

    // Test 7: polymorphism and object cache (session).
    //
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
    {
      using namespace test7;

      shared_ptr<root> r (new root (1, 1));
      shared_ptr<base> b (new base (2, 2, "b"));
      shared_ptr<root> d (new derived (3, 3, "d"));

      // Persist.
      //
      {
        session s;

        {
          transaction t (db->begin ());
          db->persist (r);
          db->persist (b);
          db->persist (d);
          t.commit ();
        }

        assert (db->load<root> (r->id) == r);
        assert (db->load<base> (b->id) == b);
        assert (db->load<root> (d->id) == d);
      }

      // Load.
      //
      {
        session s;

        transaction t (db->begin ());
        shared_ptr<root> r1 (db->load<root> (r->id));
        shared_ptr<base> b1 (db->load<base> (b->id));
        shared_ptr<derived> d1 (db->load<derived> (d->id));
        t.commit ();

        assert (db->load<root> (r->id) == r1);
        assert (db->load<base> (b->id) == b1);
        assert (db->load<root> (d->id) == d1);

        assert (!db->find<derived> (b->id));
      }

      // Query.
      //
      {
        typedef odb::query<root> query;
        typedef odb::result<root> result;

        session s;

        transaction t (db->begin ());
        shared_ptr<root> r1 (db->load<root> (r->id));
        shared_ptr<base> b1 (db->load<base> (b->id));
        shared_ptr<derived> d1 (db->load<derived> (d->id));
        t.commit ();

        {
          transaction t (db->begin ());

          result r (db->query<root> ("ORDER BY" + query::id));
          result::iterator i (r.begin ()), e (r.end ());

          assert (i != e && i.load () == r1);
          assert (++i != e && i.load () == b1);
          assert (++i != e && i.load () == d1);
          assert (++i == e);

          t.commit ();
        }
      }

      // Erase.
      //
      {
        session s;

        {
          transaction t (db->begin ());
          db->load<root> (r->id);
          db->load<root> (b->id);
          db->load<root> (d->id);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          db->erase (r);
          db->erase (b);
          db->erase (d);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          assert (!db->find<root> (r->id));
          assert (!db->find<base> (b->id));
          assert (!db->find<root> (d->id));
          t.commit ();
        }
      }
    }
#endif

    // Test 8: polymorphism and abstract bases.
    //
    {
      using namespace test8;

      base b (1, 1, "b");
      interm i (2, 2, "i", true);
      derived1 d1 (3, 3, "d1", true);
      derived2 d2 (4, 4, "d2", false);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (static_cast<root&> (d1));
        db->persist (static_cast<interm&> (d2));

        try
        {
          db->persist (i);
          assert (false);
        }
        catch (const odb::abstract_class&) {}

        try
        {
          db->persist (static_cast<base&> (i));
          assert (false);
        }
        catch (const odb::no_type_info&) {}

        t.commit ();
      }

      // Load.
      //
      {
        base vb;
        interm vi;
        derived1 vd1;
        derived2 vd2;

        transaction t (db->begin ());

        // load (id)
        //
        auto_ptr<root> pb (db->load<root> (b.id));
        auto_ptr<interm> pd1 (db->load<interm> (d1.id));
        auto_ptr<derived2> pd2 (db->load<derived2> (d2.id));

        assert (*pb == b);
        assert (*pd1 == d1);
        assert (*pd2 == d2);

        // load (id, obj)
        //
        db->load (b.id, static_cast<root&> (vb));
        db->load (d1.id, static_cast<base&> (vd1));
        db->load (d2.id, static_cast<interm&> (vd2));

        assert (vb == b);
        assert (vd1 == d1);
        assert (vd2 == d2);

        try
        {
          db->load (i.id, static_cast<root&> (vi));
          assert (false);
        }
        catch (const odb::no_type_info&) {}

        // reload (obj)
        //
        vb.num = 0;
        vd1.num = 0;
        vd2.num = 0;

        db->reload (static_cast<root&> (vb));
        db->reload (static_cast<base&> (vd1));
        db->reload (static_cast<interm&> (vd2));

        assert (vb == b);
        assert (vd1 == d1);
        assert (vd2 == d2);

        try
        {
          db->reload (static_cast<root&> (vi));
          assert (false);
        }
        catch (const odb::no_type_info&) {}

        t.commit ();
      }

      // Update.
      //
      {
        b.num++;
        b.str += 'b';
        d1.num++;
        d1.str += "d1";
        d2.num++;
        d2.str += "d1";

        {
          transaction t (db->begin ());
          db->update (static_cast<root&> (b));
          db->update (d1);
          db->update (static_cast<interm&> (d2));

          try
          {
            db->update (i);
            assert (false);
          }
          catch (const odb::abstract_class&) {}

          try
          {
            db->update (static_cast<base&> (i));
            assert (false);
          }
          catch (const odb::no_type_info&) {}

          t.commit ();
        }

        {
          transaction t (db->begin ());
          auto_ptr<base> pb (db->load<base> (b.id));
          auto_ptr<root> pd1 (db->load<root> (d1.id));
          auto_ptr<base> pd2 (db->load<base> (d2.id));
          t.commit ();

          assert (*pb == b);
          assert (*pd1 == d1);
          assert (*pd2 == d2);
        }
      }

      // Erase.
      //
      {
        transaction t (db->begin ());
        db->erase (b);
        db->erase<interm> (d1.id);
        db->erase (static_cast<root&> (d2));

        try
        {
          db->erase (i);
          assert (false);
        }
        catch (const odb::abstract_class&) {}

        try
        {
          db->erase (static_cast<base&> (i));
          assert (false);
        }
        catch (const odb::no_type_info&) {}

        t.commit ();
      }
    }

    // Test 9: polymorphism and readonly classes.
    //
    {
      using namespace test9;

      ro_root ro_r (1, 1);
      rw_base rw_b (2, 2, "b");
      ro_derived ro_d (3, 3, "d");

      rw_root rw_r (1, 1);
      ro_base ro_b (2, 2, "b");
      rw_derived rw_d (3, 3, "d");

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (ro_r);
        db->persist (rw_b);
        db->persist (ro_d);

        db->persist (rw_r);
        db->persist (ro_b);
        db->persist (rw_d);
        t.commit ();
      }

      // Update.
      //
      {
        ro_root ro_r1 (ro_r);
        rw_base rw_b1 (rw_b);
        ro_derived ro_d1 (ro_d);

        ro_base ro_b1 (ro_b);
        rw_derived rw_d1 (rw_d);

        ro_r1.num++;
        ro_r1.strs.push_back ("b");

        rw_b1.num++;
        rw_b1.strs.push_back ("b");
        rw_b1.str += "b";
        rw_b1.nums.push_back (2);
        rw_b.str += "b";
        rw_b.nums.push_back (2);

        ro_d1.num++;
        ro_d1.strs.push_back ("d");
        ro_d1.str += "d";
        ro_d1.nums.push_back (3);
        ro_d1.dnum++;
        ro_d1.dstr += "d";

        rw_r.num++;
        rw_r.strs.push_back ("b");

        ro_b1.num++;
        ro_b1.strs.push_back ("b");
        ro_b1.str += "b";
        ro_b1.nums.push_back (2);

        rw_d1.num++;
        rw_d1.strs.push_back ("d");
        rw_d1.str += "d";
        rw_d1.nums.push_back (3);
        rw_d1.dnum++;
        rw_d1.dstr += "d";
        rw_d.dnum++;
        rw_d.dstr += "d";

        {
          // These should be no-ops.
          //
          db->update (ro_r1);
          db->update (static_cast<ro_root&> (ro_d1));
          db->update (ro_b1);

          transaction t (db->begin ());
          db->update (static_cast<ro_root&> (rw_b1));
          db->update (rw_r);
          db->update (static_cast<ro_base&> (rw_d1));
          t.commit ();
        }
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<ro_root> p_ro_r (db->load<ro_root> (ro_r.id));
        auto_ptr<ro_root> p_rw_b (db->load<ro_root> (rw_b.id));
        auto_ptr<ro_root> p_ro_d (db->load<ro_root> (ro_d.id));

        auto_ptr<rw_root> p_rw_r (db->load<rw_root> (rw_r.id));
        auto_ptr<rw_root> p_ro_b (db->load<rw_root> (ro_b.id));
        auto_ptr<rw_root> p_rw_d (db->load<rw_root> (rw_d.id));
        t.commit ();

        assert (*p_ro_r == ro_r);
        assert (*p_rw_b == rw_b);
        assert (*p_ro_d == ro_d);

        assert (*p_rw_r == rw_r);
        assert (*p_ro_b == ro_b);
        assert (*p_rw_d == rw_d);
      }
    }

    // Test 10: empty polymorphic classes.
    //
    {
      using namespace test10;

      base b (1, 1);
      derived d (2, 2);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (static_cast<root&> (d));
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pb (db->load<root> (b.id));
        auto_ptr<root> pd (db->load<root> (d.id));
        t.commit ();

        assert (*pb == b);
        assert (*pd == d);
      }

      // Update.
      //
      {
        b.num++;
        d.num++;

        transaction t (db->begin ());
        db->update (static_cast<root&> (b));
        db->update (d);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pb (db->load<root> (b.id));
        auto_ptr<root> pd (db->load<root> (d.id));
        t.commit ();

        assert (*pb == b);
        assert (*pd == d);
      }
    }

    // Test 11: reuse and polymorphic inheritance.
    //
    {
      using namespace test11;

      base b (1, 1, "b");
      derived d (2, 2, "d");

      b.strs.push_back ("b");
      b.nums.push_back (1);

      d.strs.push_back ("d");
      d.nums.push_back (1);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (static_cast<base&> (d));
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<base> pd (db->load<base> (d.id));
        t.commit ();

        assert (*pb == b);
        assert (*pd == d);
      }

      // Update.
      //
      {
        b.num++;
        b.str += "b";
        b.strs.push_back ("bb");
        b.nums.push_back (2);

        d.num++;
        d.str += "d";
        d.strs.push_back ("dd");
        d.nums.push_back (2);
        d.dnum++;
        d.dstr += "d";

        transaction t (db->begin ());
        db->update (b);
        db->update (d);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<base> pd (db->load<base> (d.id));
        t.commit ();

        assert (*pb == b);
        assert (*pd == d);
      }

      // Query.
      //
      {
        typedef odb::query<base> base_query;
        typedef odb::result<base> base_result;

        typedef odb::query<derived> derived_query;
        typedef odb::result<derived> derived_result;

        transaction t (db->begin ());

        {
          base_result qr (db->query<base> (base_query::num == 2));
          base_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == b);
          assert (++i == e);
        }

        {
          derived_result qr (db->query<derived> (derived_query::num == 3));
          derived_result::iterator i (qr.begin ()), e (qr.end ());

          assert (i != e && *i == d);
          assert (++i == e);
        }

        t.commit ();
      }
    }

    // Test 12: polymorphic objects with auto id.
    //
    {
      using namespace test12;

      base b (1);
      derived d (2);

      unsigned long id1, id2;

      // Persist.
      //
      {
        transaction t (db->begin ());
        id1 = db->persist (b);
        id2 = db->persist (static_cast<root&> (d));
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pb (db->load<root> (id1));
        auto_ptr<root> pd (db->load<root> (id2));
        t.commit ();

        assert (*pb == b);
        assert (*pd == d);
      }
    }

    // Test 13: polymorphic derived without any non-container data members
    // (which results in an empty SELECT statement).
    //
    {
      using namespace test13;

      base b;
      b.nums.push_back (123);
      derived d;
      d.nums.push_back (123);
      d.strs.push_back ("abc");

      base1 b1;

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (d);
        db->persist (b1);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<root> pbr (db->load<root> (b.id));
        auto_ptr<root> pdr (db->load<root> (d.id));
        auto_ptr<base> pdb (db->load<base> (d.id));
        auto_ptr<root> pb1r (db->load<root> (b1.id));
        t.commit ();

        base& rb (static_cast<base&> (*pbr));
        derived& rd1 (static_cast<derived&> (*pdr));
        derived& rd2 (static_cast<derived&> (*pdb));
        base1 rb1 (static_cast<base1&> (*pb1r));

        assert (rb.id == b.id && rb.nums == b.nums);
        assert (rd1.id == d.id && rd1.nums == rd1.nums &&
                rd1.strs == rd1.strs);
        assert (rd2.id == d.id && rd2.nums == rd2.nums &&
                rd2.strs == rd2.strs);
        assert (rb1.id == b1.id);
      }
    }

    // Test 14: inverse pointer in polymorphic base.
    //
    {
      using namespace test14;

      derived d;
      d.num = 123;

      d.o1 = new object1;
      d.o2 = new object2;
      d.o3.push_back (new object3);
      d.o4.push_back (new object4);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (d.o1);
        db->persist (d.o2);
        db->persist (d.o3[0]);
        db->persist (d.o4[0]);
        db->persist (d);
        t.commit ();
      }

      // Load.
      //
      {
        session s;

        transaction t (db->begin ());
        object1* p1 (db->load<object1> (d.o1->id));
        object2* p2 (db->load<object2> (d.o2->id));
        object3* p3 (db->load<object3> (d.o3[0]->id));
        object4* p4 (db->load<object4> (d.o4[0]->id));
        t.commit ();

        assert (p1->d->num = d.num);
        assert (p2->d[0]->num = d.num);
        assert (p3->d[0]->num = d.num);
        assert (p4->d->num = d.num);
        delete p1->d;
      }

      // Query.
      //
      {
        typedef odb::query<object1> query;
        typedef odb::result<object1> result;

        session s;
        transaction t (db->begin ());

        result r (db->query<object1> (query::d->num == d.num));
        result::iterator i (r.begin ()), e (r.end ());

        assert (i != e && i->d->num == d.num);
        delete i.load ()->d;
        assert (++i == e);
        t.commit ();
      }

      {
        typedef odb::query<object4> query;
        typedef odb::result<object4> result;

        session s;
        transaction t (db->begin ());

        result r (db->query<object4> (query::d->num == d.num));
        result::iterator i (r.begin ()), e (r.end ());

        assert (i != e && i->d->num == d.num);
        delete i.load ()->d;
        assert (++i == e);
        t.commit ();
      }
    }

    // Test 15: LOB/long data and polymorphism.
    //
    {
      using namespace test15;

      const char data[] = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B";

      derived d;
      d.blob.assign (data, data + sizeof (data));

      // Persist.
      //
      {
        transaction t (db->begin ());
        base* b (&d);
        db->persist (b);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (d.id));
        t.commit ();

        derived* pd (dynamic_cast<derived*> (pb.get ()));
        assert (pd != 0 && pd->blob == d.blob);
      }

      // Query.
      //
      {
        typedef odb::result<base> result;

        transaction t (db->begin ());

        result r (db->query<base> ());
        result::iterator i (r.begin ()), e (r.end ());

        assert (i != e);

        derived* pd (dynamic_cast<derived*> (&*i));
        assert (pd != 0 && pd->blob == d.blob);

        assert (++i == e);
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
