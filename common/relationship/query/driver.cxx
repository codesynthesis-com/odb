// file      : common/relationship-query/query/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test relationship queries.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY
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

#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)

    //
    //
    {
      shared_ptr<country> ca (new country ("CA", "Canada"));
      shared_ptr<country> za (new country ("ZA", "South Africa"));
      shared_ptr<country> us (new country ("US", "United States"));
      shared_ptr<country> se (new country ("SE", "Sweden"));

      shared_ptr<employer> st (new employer ("Simple Tech, Inc", ca));
      shared_ptr<employer> ct (new employer ("Complex Tech, Inc", us));

      // person
      //
      shared_ptr<person> p1 (
        new person (1, "John", "Doe", 30, ca, true, za));

      shared_ptr<person> p2 (
        new person (2, "Jane", "Doe", 29, za, false, us));
      p2->husband = p1;

      shared_ptr<person> p3 (
        new person (3, "Joe", "Dirt", 31,  us, true, us));

      shared_ptr<person> p4 (
        new person (4, "Johan", "Johansen", 32, se, false, ca));

      // employee
      //
      shared_ptr<employee> e1 (
        new employee (1, "John", "Doe", 30, ca, true, za, st));

      shared_ptr<employee> e2 (
        new employee (2, "Jane", "Doe", 29, za, false, us, ct));
      e2->husband = p1;

      shared_ptr<employee> e3 (
        new employee (3, "Joe", "Dirt", 31,  us, true, us, st));

      shared_ptr<employee> e4 (
        new employee (4, "Johan", "Johansen", 32, se, false, ca, ct));

      transaction t (db->begin ());
      db->persist (ca);
      db->persist (za);
      db->persist (us);
      db->persist (se);

      db->persist (st);
      db->persist (ct);

      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      db->persist (p4);

      db->persist (e1);
      db->persist (e2);
      db->persist (e3);
      db->persist (e4);
      t.commit ();
    }

    typedef odb::query<person> p_query;
    typedef odb::result<person> p_result;

    typedef odb::query<employee> e_query;
    typedef odb::result<employee> e_result;

    // Make sure we have an independent JOIN for each relationship.
    //
    {
      session s;
      transaction t (db->begin ());

      p_result pr (db->query<person> (
                   p_query::residence.location->code == "ZA"));
      assert (size (pr) == 1);

      e_result er (db->query<employee> (
                   e_query::residence.location->code == "ZA"));
      assert (size (er) == 1);

      t.commit ();
    }

    // Test Self-JOIN.
    //
    {
      session s;
      transaction t (db->begin ());

      p_result pr (db->query<person> (p_query::husband->last_name == "Doe"));
      assert (size (pr) == 1);

      e_result er (db->query<employee> (e_query::husband->last_name == "Doe"));
      assert (size (er) == 1);

      t.commit ();
    }

    // Test query conditions from both base and derived.
    //
    {
      session s;
      transaction t (db->begin ());

      e_result r (
        db->query<employee> (
          e_query::employed_by->name == "Simple Tech, Inc" &&
          e_query::nationality->code == "US"));

      assert (size (r) == 1);

      t.commit ();
    }

    // Test second-level pointers.
    //
    {
      session s;
      transaction t (db->begin ());

      p_result r (
        db->query<person> (
          p_query::husband->residence.location == "CA"));

      assert (size (r) == 1);

      t.commit ();
    }

#endif // HAVE_CXX11 || HAVE_TR1_MEMORY

  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
