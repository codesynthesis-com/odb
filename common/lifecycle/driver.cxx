// file      : common/lifecycle/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
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

    // transient
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

    // persistent
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

    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      assert (o->str_ == "value 1");
      t.commit ();
    }

    // modified
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      o->str_ = "value 2";
      db->update (*o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      assert (o->str_ == "value 2");
      t.commit ();
    }

    // transient
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      db->erase (*o);
      t.commit ();
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
