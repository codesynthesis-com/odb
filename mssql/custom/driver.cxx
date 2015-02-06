// file      : mssql/custom/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test custom database type mapping in SQL Server.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace mssql = odb::mssql;
using namespace mssql;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    object o (1);

    o.v = variant (123);
    o.vv.push_back (variant (string (1024, 'a')));
    o.vv.push_back (variant (123));

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    o.p = point (1.1111, 2222222222.2);
    o.pv.push_back (point (1.1234, 2.2345));
    o.pv.push_back (point (3.3456, 4.4567));
    o.pv.push_back (point (0.0000001, 0.000000001)); // Scientific notation.
#endif

    o.xml = "<root x=\"1\"><a>AAA</a><b>BBB</b><c>CCC</c></root>";

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
    typedef mssql::query<object> query;
    typedef odb::result<object> result;

    {
      transaction t (db->begin ());

      // Variant comparison.
      //
      {
        result r (db->query<object> (query::v == o.v));
        assert (!r.empty ());
      }

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      // Point comparison.
      //
      {
        result r (db->query<object> (query::p == o.p));
        assert (!r.empty ());
      }

      // Point comparison using native query.
      //
      {
        result r (db->query<object> (
                    query::p + ".STEquals(" + query::_val (o.p) + ") = 1"));
        assert (!r.empty ());
      }

      // Access to individual members.
      //
      {
        result r (db->query<object> (query::p.x == o.p.x));
        assert (!r.empty ());
      }
#endif

      t.commit ();
    }

    // Update.
    //
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    o.p.x++;
    o.p.y--;
    o.pv[1].x--;
    o.pv[1].y++;
#endif

    o.xml = "<root x=\"2\"><a>BBB</a><b>CCC</b><c>DDD</c></root>";

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
