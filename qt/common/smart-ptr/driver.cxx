// file      : qt/common/smart-ptr/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt smart pointers.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QSharedPointer>
#include <QtCore/QCoreApplication>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

// Force instantiation of all QLazySharedPointer and QLazyWeakPointer
// class template members.
//
template class QLazySharedPointer<cont>;
template class QLazyWeakPointer<cont>;

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    QSharedPointer<cont> c1 (new cont (1));
    QSharedPointer<cont> c2 (new cont (2));

    // Test boolean conversion operator.
    //
    {
      assert (!QLazySharedPointer<cont> ());
      assert (!QLazyWeakPointer<cont> ());
      assert (QLazySharedPointer<cont> (c1));
      assert (QLazySharedPointer<cont> (*db, 1));
      assert (QLazyWeakPointer<cont> (c1));
      assert (QLazyWeakPointer<cont> (*db, 1));
    }

    // Test loaded () implementation.
    //
    {
      assert (QLazySharedPointer<cont> ().loaded ());
      assert (!QLazySharedPointer<cont> (c1).loaded ());
      assert (!QLazySharedPointer<cont> (*db, 1).loaded ());
      assert (QLazySharedPointer<cont> (*db, c1).loaded ());
      assert (QLazyWeakPointer<cont> ().loaded ());
      assert (!QLazyWeakPointer<cont> (c1).loaded ());
      assert (!QLazyWeakPointer<cont> (*db, 1).loaded ());
      assert (QLazyWeakPointer<cont> (*db, c1).loaded ());
    }

    // Test comparison operators.
    //
    {
      // Transient QLazySharedPointer.
      //
      assert (QLazySharedPointer<cont> () == QLazySharedPointer<cont> ());
      assert (QLazySharedPointer<cont> () != QLazySharedPointer<cont> (c1));
      assert (QLazySharedPointer<cont> (c1) != QLazySharedPointer<cont> (c2));
      assert (QLazySharedPointer<cont> (c2) == QLazySharedPointer<cont> (c2));

      // Persistent QLazySharedPointer.
      //
      QLazySharedPointer<cont> ls1 (*db, 1), ls2 (*db, c2);
      assert (ls1 != QLazySharedPointer<cont> ());
      assert (ls1 != QLazySharedPointer<cont> (c1));
      assert (ls1 == QLazySharedPointer<cont> (*db, c1));
      assert (ls1 != ls2);
      assert (ls2 == QLazySharedPointer<cont> (c2));

      // Transient QLazyWeakPointer.
      //
      assert (QLazyWeakPointer<cont> () == QLazyWeakPointer<cont> ());
      assert (QLazyWeakPointer<cont> () != QLazyWeakPointer<cont> (c1));
      assert (QLazyWeakPointer<cont> (c1) != QLazyWeakPointer<cont> (c2));
      assert (QLazyWeakPointer<cont> (c2) == QLazyWeakPointer<cont> (c2));
      assert (QLazyWeakPointer<cont> () == QLazySharedPointer<cont> ());
      assert (QLazyWeakPointer<cont> () != QLazySharedPointer<cont> (c1));
      assert (QLazyWeakPointer<cont> (c1) != QLazySharedPointer<cont> (c2));
      assert (QLazyWeakPointer<cont> (c2) == QLazySharedPointer<cont> (c2));

      // Persistent QLazyWeakPointer.
      //
      QLazyWeakPointer<cont> lw1 (*db, 1), lw2 (*db, c2);
      assert (lw1 != QLazyWeakPointer<cont> ());
      assert (lw1 != QLazyWeakPointer<cont> (c1));
      assert (lw1 == QLazyWeakPointer<cont> (*db, c1));
      assert (lw1 != lw2);
      assert (lw2 == QLazyWeakPointer<cont> (c2));
      assert (ls1 == lw1);
      assert (ls1 != QLazyWeakPointer<cont> (c1));
      assert (ls1 == QLazyWeakPointer<cont> (*db, c1));
      assert (ls1 != lw2);
      assert (ls2 == QLazyWeakPointer<cont> (c2));
    }

    // Test swap.
    //
    {
      QLazySharedPointer<cont> lx (*db, 1), ly;
      swap (lx, ly);

      assert (lx.isNull ());
      assert (ly == QLazySharedPointer<cont> (*db, c1));
    }

    // Persist.
    //
    QSharedPointer<obj> o1 (new obj (1));
    QSharedPointer<obj> o2 (new obj (2));
    QSharedPointer<obj> o3 (new obj (3));
    QSharedPointer<obj> o4 (new obj (4));

    o1->c = c1;
    o2->c = c1;
    o3->c = c2;
    o4->c = c2;

    {
      transaction t (db->begin ());

      db->persist (c1);

      db->persist (o1);
      db->persist (o2);
      db->persist (o3);
      db->persist (o4);

      db->persist (c2);

      t.commit ();
    }

    // Load.
    //
    {
      session s;
      transaction t (db->begin ());

      QSharedPointer<cont> c (db->load<cont> (1));
      QSharedPointer<obj> o (db->load<obj> (1));

      // Ensure that lazy pointers are present but not loaded.
      //
      assert (c->o.size () == 2);
      assert (!c->o[0].loaded ());
      assert (!c->o[1].loaded ());
      assert (!o->c.loaded ());

      // Ensure that the correct object IDs were loaded.
      //
      assert (c->o[0].objectId<obj> () == 1);
      assert (c->o[1].objectId<obj> () == 2);
      assert (o->c.objectId<obj> () == 1);

      // Load the lazy pointer targets ensuring that the loaded
      // targets correspond to the cached session objects.
      //
      QSharedPointer<cont> cl (o->c.load ());
      QSharedPointer<obj> ol (c->o[0].load ());

      assert (c->o[0].loaded ());
      assert (o->c.loaded ());

      assert (cl == c);
      assert (ol == o);

      t.commit ();
    }

    // Test lazy weak locking and reloading.
    //
    {
      // No session.
      //
      transaction t (db->begin ());
      QSharedPointer<cont> c (db->load<cont> (1));

      // Lock.
      //
      assert (!c->o[1].loaded ());
      QLazySharedPointer<obj> l (c->o[1].toStrongRef ());
      assert (!l.loaded ());
      assert (l.objectId<obj> () == c->o[1].objectId<obj> ());

      // Reload.
      //
      assert (!c->o[1].loaded ());

      QSharedPointer<obj> ol (c->o[1].load ());
      assert (c->o[1].loaded ());

      ol.clear ();
      assert (!c->o[1].loaded ());

      ol = c->o[1].load ();
      assert (c->o[1].loaded ());

      t.commit ();
    }

    //
    // Test QSharedPointer as a value wrapper.
    //

    {
      obj2 o1 (1);
      obj2 o2 (2);
      o2.num = QSharedPointer<unsigned long> (new unsigned long (123));

      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      QSharedPointer<obj2> o1 (db->load<obj2> (1));
      QSharedPointer<obj2> o2 (db->load<obj2> (2));
      t.commit ();

      assert (!o1->num);
      assert (o2->num && *o2->num == 123);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
