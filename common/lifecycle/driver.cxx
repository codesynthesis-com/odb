// file      : common/lifecycle/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test object state transistions.
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

    // Database operation out of transaction.
    //
    try
    {
      object o (1);
      db->persist (o);
      assert (false);
    }
    catch (const not_in_transaction&)
    {
    }

    // Transient.
    //
    try
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      assert (false);
      t.commit ();
    }
    catch (const object_not_persistent&)
    {
    }

    // Persistent.
    //
    {
      object o (1);
      o.str_ = "value 1";

      transaction t (db->begin ());
      db->persist (o);
      t.commit ();

      try
      {
        transaction t (db->begin ());
        db->persist (o);
        assert (false);
        t.commit ();
      }
      catch (const object_already_persistent&)
      {
      }
    }

    // Find.
    //
    {
      transaction t (db->begin ());

      auto_ptr<object> o1 (db->find<object> (1));
      assert (o1.get () != 0 && o1->str_ == "value 1");

      auto_ptr<object> o2 (db->find<object> (2));
      assert (o2.get () == 0);

      t.commit ();
    }

    // Find (into existing).
    //
    {
      object o;

      transaction t (db->begin ());

      assert (db->find (1, o));
      assert (o.str_ == "value 1");

      assert (!db->find (2, o));

      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      assert (o->str_ == "value 1");
      t.commit ();

      try
      {
        transaction t (db->begin ());
        auto_ptr<object> o (db->load<object> (2));
        assert (false);
        t.commit ();
      }
      catch (const object_not_persistent&)
      {
      }
    }

    // Load (into existing).
    //
    {
      object o;

      transaction t (db->begin ());
      db->load (1, o);
      assert (o.str_ == "value 1");
      t.commit ();

      try
      {
        transaction t (db->begin ());
        db->load (2, o);
        assert (false);
        t.commit ();
      }
      catch (const object_not_persistent&)
      {
      }
    }

    // Reload.
    //
    {
      object o;

      transaction t (db->begin ());
      db->load (1, o);
      o.str_ = "junk";
      db->reload (o);
      assert (o.str_ == "value 1");
      t.commit ();

      try
      {
        transaction t (db->begin ());
        o.id_ = 2;
        db->reload (o);
        assert (false);
        t.commit ();
      }
      catch (const object_not_persistent&)
      {
      }
    }

    // Modified.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      o->str_ = "value 2";
      db->update (*o);
      t.commit ();

      try
      {
        transaction t (db->begin ());
        o->id_ = 2;
        db->update (*o);
        assert (false);
        t.commit ();
      }
      catch (const object_not_persistent&)
      {
      }
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      assert (o->str_ == "value 2");
      t.commit ();
    }

    // Update of unmodified object.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      db->update (*o);
      t.commit ();
    }

    // Transient.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      db->erase (*o);
      t.commit ();

      try
      {
        transaction t (db->begin ());
        db->erase<object> (1);
        assert (false);
        t.commit ();
      }
      catch (const object_not_persistent&)
      {
      }
    }

    try
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      assert (false);
      t.commit ();
    }
    catch (const object_not_persistent&)
    {
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
