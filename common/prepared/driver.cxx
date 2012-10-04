// file      : common/prepared/driver.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test prepared query functionality.
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

    {
      person p1 ("John First",  91);
      person p2 ("John Second", 81);
      person p3 ("John Third",  71);
      person p4 ("John Fourth", 61);
      person p5 ("John Fifth",  51);

      transaction t (db->begin ());
      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      db->persist (p4);
      db->persist (p5);
      t.commit ();
    }

    typedef odb::query<person> query;
    typedef odb::prepared_query<person> prep_query;
    typedef odb::result<person> result;

    // Simple case: uncached query.
    //
    {
      transaction t (db->begin ());

      unsigned short age (90);
      prep_query pq (
        t.connection ().prepare_query<person> (
          "person-age-query",
          query::age > query::_ref (age)));

      for (unsigned short i (1); i < 6; ++i, age -= 10)
      {
        result r (pq.execute ());
        assert (size (r) == i);
      }

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
