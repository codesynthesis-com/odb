// file      : common/schema/namespace/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test database schemas (aka database namespaces).
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

    // Test database schema (aka database namespace).
    //
    using ns::object2;

    object2 o2;
    o2.id = "aaa";
    o2.nums.push_back (1);
    o2.nums.push_back (2);
    o2.nums.push_back (3);
    o2.obj1 = new object1;
    o2.obj1->str = "aaa";

    {
      transaction t (db->begin ());
      db->persist (o2.obj1);
      db->persist (o2);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object2> p2 (db->load<object2> ("aaa"));
      t.commit ();

      assert (o2 == *p2);
    }

    {
      typedef odb::query<object2> query;
      typedef odb::result<object2> result;

      transaction t (db->begin ());

      {
        result r (db->query<object2> (query::id == "aaa"));
        assert (size (r) == 1);
      }

      {
        result r (db->query<object2> (query::obj1->str == "aaa"));
        assert (size (r) == 1);
      }

      t.commit ();
    }

    {
      typedef odb::query<object_view> query;
      typedef odb::result<object_view> result;

      transaction t (db->begin ());

      result r (db->query<object_view> (query::object2::id == "aaa"));

      result::iterator i (r.begin ());
      assert (i != r.end ());
      assert (i->id2 == "aaa" && i->str == "aaa");
      assert (++i == r.end ());

      t.commit ();
    }

    {
      typedef odb::result<table_view> result;

      transaction t (db->begin ());

      result r (db->query<table_view> ());

      result::iterator i (r.begin ());
      assert (i != r.end ());
      assert (i->str == "aaa");
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
