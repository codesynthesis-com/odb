// file      : common/composite/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test composite value types.
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

    // Test basic composite functionality.
    //
    for (unsigned short i (0); i < 2; ++i)
    {
      using namespace test1;

      person p (1);
      p.name_.first = "Joe";
      p.name_.last = "Dirt";
      p.name_.title = "Mr";
      p.name_.alias.first = "Anthony";
      p.name_.alias.last = "Clean";
      p.name_.nick = "Squeaky";
      p.name_.flags.nick = true;
      p.name_.flags.alias = false;
      p.age_ = 32;

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (p);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        auto_ptr<person> p1 (db->load<person> (1));
        t.commit ();

        assert (p == *p1);
      }

      p.name_.title = "Mrs";
      p.name_.alias.first = "Anthonia";
      p.name_.flags.nick = false;
      p.name_.flags.alias = true;

      // update
      //
      {
        transaction t (db->begin ());
        db->update (p);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        auto_ptr<person> p1 (db->load<person> (1));
        t.commit ();

        assert (p == *p1);
      }

      typedef odb::query<person> query;
      typedef odb::result<person> result;

      // query
      //
      {
        transaction t (db->begin ());

        result r (db->query<person> (query::name.first == "Joe"));

        assert (!r.empty ());
        assert (*r.begin () == p);
        assert (size (r) == 1);

        t.commit ();
      }

      // query
      //
      {
        transaction t (db->begin ());

        result r (db->query<person> (query::name.flags.alias));

        assert (!r.empty ());
        assert (*r.begin () == p);
        assert (size (r) == 1);

        t.commit ();
      }

      // erase
      //
      if (i == 0)
      {
        transaction t (db->begin ());
        db->erase<person> (1);
        t.commit ();
      }
    }

    // Test composite class template instantiation.
    //
    {
      using namespace test2;

      object o (1);

      o.comp_.num = 123;
      o.comp_.str = "abc";
      o.comp_.vec.push_back (int_str_pair (123, "abc"));
      o.comp_.vec.push_back (int_str_pair (234, "bcd"));
      o.comp_.vec.push_back (int_str_pair (345, "cde"));

      o.pair_.first = 123;
      o.pair_.second = "abc";

      o.vec_.push_back (int_str_pair (123, "abc"));
      o.vec_.push_back (int_str_pair (234, "bcd"));
      o.vec_.push_back (int_str_pair (345, "cde"));

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
        auto_ptr<object> o1 (db->load<object> (1));
        t.commit ();

        assert (o == *o1);
      }
    }

    // Test empty column name.
    //
    {
      using namespace test3;

      object o (1);
      o.c_.str = "abc";

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
        auto_ptr<object> o1 (db->load<object> (1));
        t.commit ();

        assert (o == *o1);
      }
    }

    // Test composite definition inside object.
    {
      using namespace test4;

      object o (1);
      o.str ("abc");
      o.x (123);
      o.y (234);

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
        auto_ptr<object> o1 (db->load<object> (1));
        t.commit ();

        assert (o == *o1);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
