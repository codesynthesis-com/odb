// file      : pgsql/custom/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test custom database type mapping in PostgreSQL.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    object o (1);
    o.p = point (1.1111, 2222222222.2);
    o.pv.push_back (point (1.1234, 2.2345));
    o.pv.push_back (point (3.3456, 4.4567));
    o.pv.push_back (point (0.0000001, 0.000000001)); // Scientific notation.

    o.n1 = "23.5154";
    o.n2 = "235154";
    o.n3 = "2222222222222222222222222222.111111111111111111111111111111";

    o.iv.push_back (123);
    o.iv.push_back (234);
    o.iv.push_back (-345);

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o == *o1);
    }

    // Query.
    //
    typedef pgsql::query<object> query;
    typedef odb::result<object> result;

    {
      transaction t (db->begin ());

      // Point comparison.
      //
      {
        result r (db->query<object> (query::p == o.p));
        assert (!r.empty ());
      }

      // Point comparison using native query.
      //
      {
        result r (db->query<object> (query::p + "~=" + query::_val (o.p)));
        assert (!r.empty ());
      }

      // Access to individual members.
      //
      {
        result r (db->query<object> (query::p.x == o.p.x));
        assert (!r.empty ());
      }

      t.commit ();
    }

    // Update.
    //
    o.p.x++;
    o.p.y--;
    o.pv[1].x--;
    o.pv[1].y++;
    o.n3 += "999";
    o.iv[0]++;
    o.iv.pop_back ();

    {
      transaction t (db->begin ());
      db->update (o);
      t.commit ();
    }

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
