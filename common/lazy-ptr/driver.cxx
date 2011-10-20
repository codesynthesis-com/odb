// file      : common/lazy-ptr/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test lazy object pointers.
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

auto_ptr<obj2>
create (unsigned int id)
{
  auto_ptr<obj2> r (new obj2 (id));
  return r;
}

lazy_auto_ptr<obj2>
create (database& db, unsigned int id)
{
  lazy_auto_ptr<obj2> r (db, id);
  return r;
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Raw.
    //
    {
      typedef cont1 cont;
      typedef obj1 obj;

      // persist
      //
      obj* o1 (new obj (1));

      {
        transaction t (db->begin ());
        db->persist (o1);
        t.commit ();
      }

      auto_ptr<cont> c1 (new cont (1));
      auto_ptr<cont> c2 (new cont (2));

      lazy_ptr<obj> lo1 (*db, 1);
      obj* o2 (new obj (2));

      obj* o3 (new obj (3));
      obj* o4 (new obj (4));

      c1->o.push_back (lo1);
      c1->o.push_back (o2);
      c2->o.push_back (o3);
      c2->o.push_back (o4);

      // Test pointer comparison.
      //
      assert (lazy_ptr<obj> () == lazy_ptr<obj> ());
      assert (lazy_ptr<obj> (o1) != lazy_ptr<obj> ());
      assert (lo1 != lazy_ptr<obj> ());
      assert (lazy_ptr<obj> (o1) == lazy_ptr<obj> (o1));
      assert (lo1 == lazy_ptr<obj> (*db, o1));
      assert (lo1 != lazy_ptr<obj> (*db, o2));

      delete o1;

      {
        transaction t (db->begin ());

        db->persist (o2);
        db->persist (o3);
        db->persist (o4);

        db->persist (*c1);
        db->persist (*c2);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        auto_ptr<cont> c (db->load<cont> (1));
        obj* o (db->load<obj> (1));

        // Not loaded.
        //
        assert (c->o.size () == 2);
        assert (!c->o[0].loaded ());
        assert (!c->o[1].loaded ());

        assert (!o->c.loaded ());

        // Correct object ids.
        //
        assert (c->o[0].object_id<obj> () == o->id);
        assert (o->c.object_id<cont> () == c->id);

        // Load.
        //
        cont* cl (o->c.load ());
        obj* ol (c->o[0].load ());

        assert (cl == c.get ());
        assert (ol == o);

        // Test unload/reload.
        //
        o->c.unload ();
        assert (!o->c.loaded ());
        o->c.load ();
        assert (o->c.loaded ());

        t.commit ();
      }
    }

    // Auto pointer.
    //
    {
      typedef cont2 cont;
      typedef obj2 obj;

      // persist
      //
      {
        auto_ptr<obj> o1 (new obj (1));
        transaction t (db->begin ());
        db->persist (*o1);
        t.commit ();
      }

      auto_ptr<cont> c1 (new cont (1));
      auto_ptr<cont> c2 (new cont (2));

      lazy_auto_ptr<obj> lo1 = create (*db, 1);
      lo1 = create (*db, 1);

      c1->o = lo1;
      c2->o = create (2);

      {
        transaction t (db->begin ());

        db->persist (*c2->o);

        db->persist (*c1);
        db->persist (*c2);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        auto_ptr<cont> c (db->load<cont> (1));
        obj* o (db->load<obj> (1));

        // Not loaded.
        //
        assert (!c->o.loaded ());
        assert (!o->c.loaded ());

        // Correct object ids.
        //
        assert (c->o.object_id<obj> () == o->id);
        assert (o->c.object_id<cont> () == c->id);

        // Load.
        //
        cont* cl (o->c.load ());
        const auto_ptr<obj>& ol (c->o.load ());

        assert (cl == c.get ());
        assert (ol.get () == o);

        t.commit ();
      }

      // unload/reload
      //
      {
        // No session.
        transaction t (db->begin ());
        auto_ptr<cont> c (db->load<cont> (1));

        assert (!c->o.loaded ());
        c->o.load ();
        assert (c->o.loaded ());
        c->o.unload ();
        assert (!c->o.loaded ());
        c->o.load ();
        assert (c->o.loaded ());

        t.commit ();
      }
    }

    // TR1.
    //
#ifdef HAVE_TR1_MEMORY
    {
      using namespace ::tr1;

      // persist
      //
      shared_ptr<cont> c1 (new cont (1));

      {
        transaction t (db->begin ());
        db->persist (c1);
        t.commit ();
      }

      lazy_shared_ptr<cont> lc1 (*db, 1);
      shared_ptr<cont> c2 (new cont (2));

      shared_ptr<obj> o1 (new obj (1));
      shared_ptr<obj> o2 (new obj (2));

      shared_ptr<obj> o3 (new obj (3));
      shared_ptr<obj> o4 (new obj (4));

      o1->c = lc1;
      o2->c = lc1;
      o3->c = c2;
      o4->c = c2;

      // Test pointer comparison.
      //
      assert (lazy_shared_ptr<cont> () == lazy_shared_ptr<cont> ());
      assert (lazy_shared_ptr<cont> (c1) != lazy_shared_ptr<cont> ());
      assert (lc1 != lazy_shared_ptr<cont> ());
      assert (lazy_shared_ptr<cont> (c1) == lazy_shared_ptr<cont> (c1));
      assert (lc1 == lazy_shared_ptr<cont> (*db, c1));
      assert (lc1 != lazy_shared_ptr<cont> (*db, c2));

      {
        transaction t (db->begin ());

        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        db->persist (o4);

        db->persist (c2);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        shared_ptr<cont> c (db->load<cont> (1));
        shared_ptr<obj> o (db->load<obj> (1));

        // Not loaded.
        //
        assert (c->o.size () == 2);
        assert (!c->o[0].loaded ());
        assert (!c->o[1].loaded ());

        assert (!o->c.loaded ());

        // Correct object ids.
        //
        assert (c->o[0].object_id<obj> () == o->id);
        assert (o->c.object_id<cont> () == c->id);

        // Load.
        //
        shared_ptr<cont> cl (o->c.load ());
        shared_ptr<obj> ol (c->o[0].load ());

        assert (cl == c);
        assert (ol == o);

        t.commit ();
      }

      // Test lazy weak locking and reloading.
      //
      {
        // No session.
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
    }
#endif
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
