// file      : common/composite/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
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
using namespace odb;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

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

    //
    //
    {
      transaction t (db->begin ());
      db->persist (p);
      t.commit ();
    }

    //
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

    //
    //
    {
      transaction t (db->begin ());
      db->update (p);
      t.commit ();
    }

    //
    //
    {
      transaction t (db->begin ());
      auto_ptr<person> p1 (db->load<person> (1));
      t.commit ();

      assert (p == *p1);
    }

    typedef odb::query<person> query;
    typedef odb::result<person> result;

    //
    //
    {
      transaction t (db->begin ());

      result r (db->query<person> (query::name::first == "Joe"));

      assert (r.size () == 1);
      assert (*r.begin () == p);

      t.commit ();
    }

    //
    //
    {
      transaction t (db->begin ());

      result r (db->query<person> (query::name::flags::alias));

      assert (r.size () == 1);
      assert (*r.begin () == p);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
