// file      : common/query/one/driver.cxx
// copyright : Copyright (c) 2009-2014 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query one support.
//
// We assume that other tests in common/query/ exercise a variety of
// different kinds of queries. Here we are concerned with what is
// specific to query_one() and query_value().
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
    odb::database_id db_id (db->id ());

    transaction t (db->begin ());

    // query_one()
    //
    {
      auto_ptr<object> o (db->query_one<object> ());
      assert (o.get () == 0);
    }

    {
      object o (4);
      assert (!db->query_one<object> (o) && o.id_ == 4 && o.str_.empty ());
    }

    /*
    {
      object o (db->query_value<object> ());
      assert (false);
    }
    */

    object o (1);
    o.str_ = "value 1";
    db->persist (o);

    {
      auto_ptr<object> o (db->query_one<object> ());
      assert (o.get () != 0 && o->str_ == "value 1");
    }

    {
      object o;
      assert (db->query_one<object> (o) && o.str_ == "value 1");
    }

    {
      object o (db->query_value<object> ());
      assert (o.str_ == "value 1");
    }

    // query_one(const char*)
    //
    const char* q1_c (db_id == odb::id_oracle ? "\"id\" = 1" : "id = 1");
    const char* q0_c (db_id == odb::id_oracle ? "\"id\" = 2" : "id = 2");

    {
      auto_ptr<object> o (db->query_one<object> (q1_c));
      assert (o.get () != 0 && o->str_ == "value 1");
    }

    {
      auto_ptr<object> o (db->query_one<object> (q0_c));
      assert (o.get () == 0);
    }

    {
      object o;
      assert (db->query_one<object> (q1_c, o) && o.str_ == "value 1");
    }

    {
      object o (4);
      assert (!db->query_one<object> (q0_c, o) &&
              o.id_ == 4 && o.str_.empty ());
    }

    {
      object o (db->query_value<object> (q1_c));
      assert (o.str_ == "value 1");
    }

    // query_one(std::string)
    //
    string q1_s (q1_c);
    string q0_s (q0_c);

    {
      auto_ptr<object> o (db->query_one<object> (q1_s));
      assert (o.get () != 0 && o->str_ == "value 1");
    }

    {
      auto_ptr<object> o (db->query_one<object> (q0_s));
      assert (o.get () == 0);
    }

    {
      object o;
      assert (db->query_one<object> (q1_s, o) && o.str_ == "value 1");
    }

    {
      object o (4);
      assert (!db->query_one<object> (q0_s, o) &&
              o.id_ == 4 && o.str_.empty ());
    }

    {
      object o (db->query_value<object> (q1_s));
      assert (o.str_ == "value 1");
    }

    // query_one(odb::query)
    //
    typedef odb::query<object> query;

    query q1 (query::id == 1);
    query q0 (query::id == 2);

    {
      auto_ptr<object> o (db->query_one<object> (q1));
      assert (o.get () != 0 && o->str_ == "value 1");
    }

    {
      auto_ptr<object> o (db->query_one<object> (q0));
      assert (o.get () == 0);
    }

    {
      object o;
      assert (db->query_one<object> (q1, o) && o.str_ == "value 1");
    }

    {
      object o (4);
      assert (!db->query_one<object> (q0, o) && o.id_ == 4 && o.str_.empty ());
    }

    {
      object o (db->query_value<object> (q1));
      assert (o.str_ == "value 1");
    }

    // Assertion on more than one element.
    //
    {
      object o (2);
      o.str_ = "value 2";
      db->persist (o);
    }

    /*
    {
      auto_ptr<object> o (db->query_one<object> ());
      assert (false);
    }
    */

    /*
    {
      object o;
      db->query_one<object> (o);
      assert (false);
    }
    */

    /*
    {
      object o (db->query_value<object> ());
      assert (false);
    }
    */
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
