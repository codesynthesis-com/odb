// file      : common/container/change-tracking/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test change-tracking containers.
//

#include <common/config.hxx> // HAVE_CXX11

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#ifdef HAVE_CXX11
#  include <utility> // std::move
#endif

#include <odb/tracer.hxx>
#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

struct counting_tracer: odb::tracer
{
  void
  reset (transaction& tr) {u = i = d = s = t = 0; tr.tracer (*this);}

  virtual void
  execute (odb::connection&, const char* stmt)
  {
    string p (stmt, 6);
    if (p == "UPDATE")
      u++;
    else if (p == "INSERT")
      i++;
    else if (p == "DELETE")
      d++;
    else if (p == "SELECT")
      s++;
    t++;
  }

  size_t u, i, d, s, t;
};

static counting_tracer tr;

// Compilation test: instantiate all the functions. In C++11 mode only
// do this if we have a fairly conforming compiler that implements the
// complete std::vector interface.
//

#ifndef _RWSTD_NO_CLASS_PARTIAL_SPEC
#if !defined (HAVE_CXX11) || \
  (defined (__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 7)
struct item {};
template class odb::vector<item>;
template class odb::vector_iterator<odb::vector<item>,
                                    std::vector<item>::iterator>;
template class odb::vector_iterator<odb::vector<item>,
                               std::vector<item>::reverse_iterator>;
#endif
#endif

void
f (const std::vector<int>&) {}

int
main (int argc, char* argv[])
{
  try
  {
    // Test extended interface.
    //
    {
      typedef odb::vector<int> vector;

      vector ov;
      std::vector<int> sv;
      f (ov);               // Implicit conversion to std::vector.
      vector ov1 (sv);      // Initialization from std::vector.
      ov = sv;              // Assignement from std::vector.

      // Container comparison.
      //
      if (ov != ov1 ||
          ov != sv  ||
          sv != ov1)
        ov.clear ();

      // Iterator comparison/conversion.
      //
      vector::const_iterator i (ov.begin ());
      if (i != ov.end ())
        i = ov.end ();

      // Things are just really borken in Sun CC, no matter which STL
      // you use.
      //
#ifndef __SUNPRO_CC
      vector::const_reverse_iterator j (ov.rbegin ());
      if (j != ov.rend ())
        j = ov.rend ();
#endif
    }

    auto_ptr<database> db (create_database (argc, argv));

    // Test traits logic.
    //
    {
      object o ("1");
      o.i = 123;
      o.s.push_back ("a");

      assert (!o.s._tracking ());

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      assert (o.s._tracking ());

      // load
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> ("1"));
        assert (p->s._tracking ());
        t.commit ();
      }

      // update
      //
      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      assert (o.s._tracking ());

      // erase
      //
      {
        transaction t (db->begin ());
        db->erase (o);
        t.commit ();
      }

      assert (!o.s._tracking ());
    }

    // Test change tracking.
    //
    object o ("1");
    o.i = 123;
    o.s.push_back ("a");

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    // push_back/pop_back
    //
    {
      o.s.push_back ("b"); // insert

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.i == 1 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.pop_back ();
      o.s.push_back ("c"); // update

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 2 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.pop_back ();
      for (int i (0); i != 1024; ++i)
        o.s.push_back ("x"); // realloc

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 2 && tr.i == 1023 && tr.t == 1025);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      for (int i (0); i != 1024; ++i)
        o.s.pop_back (); // delete

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.d == 1 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.push_back ("b");
      o.s.pop_back (); // no-op

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.t == 1);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // insert
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.insert (o.s.begin (), "a1"); // insert front

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.i == 1 && tr.t == 4);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.insert (o.s.begin () + 1, "a2"); // insert middle

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.i == 1 && tr.t == 4);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.insert (o.s.end (), "b1"); // insert back

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.i == 1 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // erase
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");
      o.s.push_back ("d");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.erase (o.s.begin ()); // erase front

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 4 && tr.d == 1 && tr.t == 5);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.erase (o.s.begin () + 1); // erase middle

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 2 && tr.d == 1 && tr.t == 3);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.erase (o.s.end () - 1); // erase back

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.d == 1 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // modify
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");
      o.s.push_back ("d");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.modify (1) += 'b';
      o.s.modify_at (2) += 'c';

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.t == 3);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.modify_front () += 'a';
      o.s.modify_back () += 'd';

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.t == 3);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.begin ().modify () += 'a';
#ifndef _RWSTD_NO_CLASS_PARTIAL_SPEC
      o.s.rbegin ().modify () += 'c';
#endif

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
#ifndef _RWSTD_NO_CLASS_PARTIAL_SPEC
      assert (tr.u == 3 && tr.t == 3);
#else
      assert (tr.u == 2 && tr.t == 2);
#endif
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

#ifndef _RWSTD_NO_CLASS_PARTIAL_SPEC
    {
      (o.s.rbegin () + 1).modify (1) += 'a';
      (o.s.rbegin () + 1).modify (-1) += 'c';

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.t == 3);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }
#endif

    {
      o.s.mbegin ();

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 5 && tr.t == 5);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // clear
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.clear ();

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.d == 1 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // assign
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.assign (4, "x");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 4 && tr.i == 1 && tr.t == 5);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // resize
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.pop_back ();
      o.s.resize (4, "x"); // expand

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 2 && tr.i == 1 && tr.t == 3);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.push_back ("y");
      o.s.resize (3);      // shrink

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 1 && tr.d == 1 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // Transaction rollback.
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      {
        o.s.push_back ("d");

        transaction t (db->begin ());
        db->update (o);
        t.rollback ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (o);
        assert (tr.u == 1 && tr.i == 4 && tr.d == 1 && tr.t == 6);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (o);
        assert (tr.u == 1 && tr.t == 1);
        t.commit ();
      }
    }

    // Armed copy.
    //
    {
      auto_ptr<object> c;

      {
        o.s.pop_back ();

        transaction t (db->begin ());
        db->update (o);
        c.reset (new object (o));
        t.rollback ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (c);
        assert (tr.u == 1 && tr.i == 3 && tr.d == 1 && tr.t == 5);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (c);
        assert (tr.u == 1 && tr.t == 1);
        t.commit ();
      }
    }

    // Armed swap.
    //
    {
      object c (o);

      {
        o.s.push_back ("d");

        transaction t (db->begin ());
        db->update (o);
        assert (o.s._tracking () && !c.s._tracking ());
        c.s.swap (o.s);
        assert (!o.s._tracking () && c.s._tracking ());
        t.rollback ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (c);
        assert (tr.u == 1 && tr.i == 4 && tr.d == 1 && tr.t == 6);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (c);
        assert (tr.u == 1 && tr.t == 1);
        t.commit ();
      }
    }

    // Armed move.
    //
#ifdef HAVE_CXX11
    {
      auto_ptr<object> c;

      {
        o.s.pop_back ();

        transaction t (db->begin ());
        db->update (o);
        assert (o.s._tracking ());
        c.reset (new object (std::move (o)));
        assert (!o.s._tracking () && c->s._tracking ());
        t.rollback ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (c);
        assert (tr.u == 1 && tr.i == 2 && tr.d == 1 && tr.t == 4);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        tr.reset (t);
        db->update (c);
        assert (tr.u == 1 && tr.t == 1);
        t.commit ();
      }
    }
#endif

    // Test mixing "smart" and "dumb" container (specifically, erase(obj)).
    //
    {
      mix_object o (1);
      o.ov.assign (3, 123);
      o.sv.assign (3, 123);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->erase (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }
    }

    // Test using change tracking container as inverse member.
    //
    {
      inv_object1 o1;
      inv_object2 o2;
      o1.o2 = &o2;

      {
        transaction t (db->begin ());
        db->persist (o2);
        db->persist (o1);
        t.commit ();
      }

      assert (!o2.o1._tracking ());

      {
        session s;
        transaction t (db->begin ());
        auto_ptr<inv_object1> p1 (db->load<inv_object1> (o1.id_));
        auto_ptr<inv_object2> p2 (db->load<inv_object2> (o2.id_));
        assert (p2->o1[0] == p1.get ());
        assert (!p2->o1._tracking ());
        t.commit ();
      }
    }

    // Test read-only values.
    {
      ro_object o (1);
      o.v.push_back (ro_value (1, 1));
      o.v.push_back (ro_value (2, 2));
      o.v.push_back (ro_value (3, 3));

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      o.v.erase (o.v.begin ());

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        assert (db->load<ro_object> (1)->v == o.v);
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
