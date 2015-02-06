// file      : common/inheritance/transient/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test transient inheritance of objects, composite value types, and views.
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

    object o;
    o.num = 1;
    o.str = "abc";
    o.strs.push_back ("abc 1");
    o.strs.push_back ("abc 2");
    o.c.num = 11;
    o.c.str = "comp abc";
    o.c.nums.push_back (111);
    o.c.nums.push_back (112);

    // persist
    //
    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    // load & check
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> p (db->load<object> (o.id_));
      t.commit ();

      assert (*p == o);
    }

    // view
    //
    {
      typedef odb::query<view> query;
      typedef odb::result<view> result;

      transaction t (db->begin ());

      result r (db->query<view> (query::id == o.id_));
      result::iterator i (r.begin ());
      assert (i != r.end () && i->num == o.num && i->str == o.str);
      assert (++i == r.end ());

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
