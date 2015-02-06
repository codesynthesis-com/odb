// file      : qt/common/containers/change-tracking/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test change-tracking Qt containers.
//

#include <common/config.hxx> // HAVE_CXX11

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#ifdef HAVE_CXX11
#  include <utility> // std::move
#endif

#include <QtCore/QtGlobal> // QT_VERSION, Q_FOREACH
#include <QtCore/QCoreApplication>

#include <odb/tracer.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <odb/qt/list-iterator.hxx>
#include <odb/qt/mutable-list-iterator.hxx>

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

// Compilation test: instantiate all the functions. Only do this if we
// have a fairly recent version of Qt, otherwise some underlying
// functions will be missing.
//
#if QT_VERSION >= 0x050000
template class QOdbList<short>;
template class QOdbListIteratorImpl<QOdbList<short> >;
template class QOdbListIterator<short>;
template class QMutableOdbListIterator<short>;
#endif

void
f (const QList<int>&) {}

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    // Test extended interface.
    //
    {
      typedef QOdbList<int> list;

      list ol;
      QList<int> ql;
      f (ol);             // Implicit conversion to QList.
      list ol1 (ql);      // Initialization from QList.
      ol = ql;            // Assignement from QList.

      // Container comparison.
      //
      if (ol != ol1 ||
          ol != ql  ||
          ql != ol1)
        ol.clear ();

      // Container operators.
      //
      ol += ol1;
      ol += ql;
      ol = ol1 + ql;
      ol = ql + ol1;

      // Iterator comparison/conversion.
      //
#ifndef QT_STRICT_ITERATORS
      list::const_iterator i (ol.begin ());
      if (i != ol.end ())
        i = ol.end ();
#endif

      Q_FOREACH (const int& i, ol)
        cerr << i;
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
      o.s.modify_back () += 'd';

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

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 2 && tr.t == 2);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

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
      QList<QString> v;
      v.push_back ("1");
      v.push_back ("2");
      v.push_back ("3");
      v.push_back ("4");
      o.s = v;

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 4 && tr.i == 1 && tr.t == 5);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    // removeOne/removeAll
    //
    {
      o.s.clear ();
      o.s.push_back ("a");
      o.s.push_back ("a");
      o.s.push_back ("b");
      o.s.push_back ("c");
      o.s.push_back ("a");
      o.s.push_back ("d");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.removeOne ("c");
      assert (o.s.size () == 5 && o.s[3] == "a");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.d == 1 && tr.t == 4);
      assert (*db->load<object> ("1") == o);
      t.commit ();
    }

    {
      o.s.removeAll ("a");
      assert (o.s.size () == 2 && o.s[0] == "b" && o.s[1] == "d");

      transaction t (db->begin ());
      tr.reset (t);
      db->update (o);
      assert (tr.u == 3 && tr.d == 1 && tr.t == 4);
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
#if QT_VERSION >= 0x040800
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
#endif

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
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
