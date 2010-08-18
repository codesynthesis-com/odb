// file      : mysql/truncation/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test insufficient buffer/truncation handling.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

#include <common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // The default pre-allocated buffer is 512 bytes long.
    //
    string long_str (640, 'c'); // This will get the buffer to 1024
    string longer_str (1025, 'b');

    // Run persist/load so that the initial bindings are established
    // (version == 0).
    //
    {
      object1 o (1);
      o.str_ = "test string";

      transaction t (db->begin_transaction ());
      db->persist (o);
      db->load (1, o);
      t.commit ();
    }

    {
      object2 o (2);
      o.str_ = "test string";

      transaction t (db->begin_transaction ());
      db->persist (o);
      db->load (2, o);
      t.commit ();
    }

    // Store/load the long string which should trigger buffer growth.
    //
    {
      object1 o (3);
      o.str_ = long_str;

      transaction t (db->begin_transaction ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin_transaction ());
      auto_ptr<object2> o (db->load<object2> (3));
      assert (o->str_ == long_str);
      t.commit ();
    }

    // Store/load longer string.
    //
    {
      object1 o (3);
      o.str_ = longer_str;

      transaction t (db->begin_transaction ());
      db->store (o);
      t.commit ();
    }

    {
      transaction t (db->begin_transaction ());
      auto_ptr<object2> o (db->load<object2> (3));
      assert (o->str_ == longer_str);
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
