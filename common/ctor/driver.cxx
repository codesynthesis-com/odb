// file      : common/ctor/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test support for persistent objects without default constructors.
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
    typedef odb::query<person> query;
    typedef odb::result<person> result;

    auto_ptr<database> db (create_database (argc, argv));

    person p1 ("John", "Doe", 30);
    person p2 ("Jane", "Doe", 29);
    person p3 ("Joe", "Dirt", 31);

    {
      transaction t (db->begin ());

      db->persist (p1);
      db->persist (p2);
      db->persist (p3);

      t.commit ();
    }

    {
      person p ("", "", 0);

      transaction t (db->begin ());

      db->load (p1.id_, p);

      assert (p.first_ == p1.first_);
      assert (p.last_ == p1.last_);
      assert (p.age_ == p1.age_);

      result r (db->query<person> (query::age < 30));

      assert (!r.empty ());

      result::iterator i (r.begin ());
      i.load (p);
      assert (p.first_ == "Jane");
      assert (p.last_ == "Doe");
      assert (p.age_ == 29);

      assert (size (r) == 1);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
