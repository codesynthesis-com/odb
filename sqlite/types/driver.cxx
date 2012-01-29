// file      : sqlite/types/driver.cxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test SQLite type conversion.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <common/buffer.hxx>
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

    object o (1);

    o.bool_ = true;
    o.integer_ = -123456;
    o.real_ = 1.123;

    string long_str (2040, 'l');

    o.text_ = long_str;
    o.blob_.assign (long_str.c_str (), long_str.c_str () + long_str.size ());

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    //
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o == *o1);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
