// file      : boost/common/smart-ptr/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test boost smart pointers.
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

using namespace odb::boost;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  using boost::shared_ptr;

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    shared_ptr<cont> c1 (new cont (1));
    {
      transaction t (db->begin ());
      db->persist (c1);
      t.commit ();
    }

    // Test comparison operators.
    //
    {
      assert (lazy_shared_ptr<cont> () == lazy_shared_ptr<cont> ());
      assert (lazy_shared_ptr<cont> () != lazy_shared_ptr<cont> (c1));
      assert (lazy_shared_ptr<cont> (c1) == lazy_shared_ptr<cont> (c1));

      lazy_shared_ptr<cont> lc1 (*db, 1);
      assert (lc1 != lazy_shared_ptr<cont> ());
      assert (lc1 == lazy_shared_ptr<cont> (*db, c1));

      shared_ptr<cont> c2 (new cont (2));
      assert (lc1 != lazy_shared_ptr<cont> (*db, c2));
    }

    // Test swap.
    //
    {
      lazy_shared_ptr<cont> lx (*db, 1), ly;
      assert (lx == lazy_shared_ptr<cont> (*db, c1));

      swap (lx, ly);
      assert (lx == lazy_shared_ptr<cont> ());
      assert (ly == lazy_shared_ptr<cont> (*db, c1));
    }

    // Test assignment from auto_ptr.
    //
    {
      cont* p = new cont (3);
      auto_ptr<cont> a (p);
      lazy_shared_ptr<cont> l;
      l = a;

      assert (l.get() == p);
      assert (!a.get ());
    }

    shared_ptr<obj> o1 (new obj (1));
    shared_ptr<obj> o2 (new obj (2));
    shared_ptr<obj> o3 (new obj (3));
    shared_ptr<obj> o4 (new obj (4));
    shared_ptr<cont> c2 (new cont (2));

    o1->c = c1;
    o2->c = c1;
    o3->c = c2;
    o4->c = c2;

    // Persist.
    //
    {
      transaction t (db->begin ());

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

      shared_ptr<cont> c (db->load<cont> (1));
      shared_ptr<obj> o (db->load<obj> (1));

      // Ensure that lazy pointers are present but not loaded.
      //
      assert (c->o.size () == 2);
      assert (!c->o[0].loaded ());
      assert (!c->o[1].loaded ());
      assert (!o->c.loaded ());

      // Ensure that the correct object IDs were loaded.
      //
      assert (c->o[0].object_id<obj> () == 1);
      assert (c->o[1].object_id<obj> () == 2);
      assert (o->c.object_id<obj> () == 1);

      // Load the lazy pointer targets ensuring that the loaded
      // targets correspond to the cached session objects.
      //
      shared_ptr<cont> cl (o->c.load ());
      shared_ptr<obj> ol (c->o[0].load ());

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
      shared_ptr<cont> c (db->load<cont> (1));

      // Lock.
      //
      assert (!c->o[1].loaded ());
      lazy_shared_ptr<obj> l (c->o[1].lock ());
      assert (!l.loaded ());
      assert (l.object_id<obj> () == c->o[1].object_id<obj> ());

      // Reload.
      //
      assert (!c->o[1].loaded ());

      shared_ptr<obj> ol (c->o[1].load ());
      assert (c->o[1].loaded ());

      ol.reset ();
      assert (!c->o[1].loaded ());

      ol = c->o[1].load ();
      assert (c->o[1].loaded ());

      t.commit ();
    }

    //
    // Test shared_ptr as a value wrapper.
    //

    {
      obj2 o1 (1);
      obj2 o2 (2);
      o2.str.reset (new string ("abc"));

      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      shared_ptr<obj2> o1 (db->load<obj2> (1));
      shared_ptr<obj2> o2 (db->load<obj2> (2));
      t.commit ();

      assert (!o1->str);
      assert (o2->str && *o2->str == "abc");
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
