// file      : common/wrapper/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test wrapper machinery.
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

    // Test 1: simple values.
    //
    {
      using namespace test1;

      unsigned long id1, id2;
      {
        object1 o1;
        object2 o2;

        o1.num.reset (new int (123));
        o1.nstrs.push_back (nullable_string ());
        o1.nstrs.push_back (nullable_string ("123"));

#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
        o2.sstrs.push_back (str_sptr ());
        o2.sstrs.push_back (str_sptr (new string ("123")));
#endif

        transaction t (db->begin ());
        id1 = db->persist (o1);
        id2 = db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object1> o1 (db->load<object1> (id1));
        auto_ptr<object2> o2 (db->load<object2> (id2));
        t.commit ();

        assert (*o1->num == 123);
        assert (o1->str.get () == 0);
        assert (o1->nstr.null ());
        assert (o1->nstrs[0].null ());
        assert (o1->nstrs[1].get () == "123");

#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
        assert (!o2->sstr);
        assert (!o2->sstrs[0]);
        assert (*o2->sstrs[1] == "123");
#endif
      }
    }

    //
    // Composite values.
    //
    unsigned long id;
    {
      comp_object co;

      co.c1.reset (new comp1 ("123", 123));
      co.vc1.push_back (comp1 ("1", 1));
      co.vc1.push_back (comp1 ("2", 2));
      co.vc1.push_back (comp1 ("3", 3));

      co.c2.reset (new comp2 ("123", 123));
      co.c2->strs.push_back ("1");
      co.c2->strs.push_back ("2");
      co.c2->strs.push_back ("3");

      {
        transaction t (db->begin ());
        id = db->persist (co);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<comp_object> o (db->load<comp_object> (id));
        t.commit ();

        assert (*o->c1 == *co.c1);
        assert (o->vc1 == co.vc1);
        assert (*o->c2 == *co.c2);
      }
    }

    //
    // Containers.
    //
    {
      cont_object co;

      co.nums.reset (new vector<int>);
      co.nums->push_back (1);
      co.nums->push_back (2);
      co.nums->push_back (3);

      co.c.num = 123;
      co.c.strs.reset (new vector<string>);
      co.c.strs->push_back ("1");
      co.c.strs->push_back ("2");
      co.c.strs->push_back ("3");

      {
        transaction t (db->begin ());
        id = db->persist (co);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<cont_object> o (db->load<cont_object> (id));
        t.commit ();

        assert (*o->nums == *co.nums);
        assert (o->c == co.c);
      }
    }

    // Test 5: composite NULL values.
    //
    {
      using namespace test5;

      object o1, o2;

      o1.v.push_back (nullable<comp> ());

      o2.p.reset (new comp (1, "a"));
      o2.n = comp (2, "b");
      o2.v.push_back (comp (3, "c"));

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p1 (db->load<object> (o1.id));
        auto_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (p1->p.get () == 0);
        assert (!p1->n);
        assert (!p1->v[0]);

        assert (p2->p.get () != 0 && *p2->p == *o2.p);
        assert (p2->n && *p2->n == *o2.n);
        assert (p2->v[0] && *p2->v[0] == *o2.v[0]);
      }

      // Update.
      //
      {
        o1.p.reset (new comp (1, "a"));
        o1.n = comp (2, "b");
        o1.v[0] = comp (3, "c");

        o2.p.reset ();
        o2.n.reset ();
        o2.v[0].reset ();

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

        assert (p1->p.get () != 0 && *p1->p == *o1.p);
        assert (p1->n && *p1->n == *o1.n);
        assert (p1->v[0] && *p1->v[0] == *o1.v[0]);

        assert (p2->p.get () == 0);
        assert (!p2->n);
        assert (!p2->v[0]);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
