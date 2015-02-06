// file      : mssql/query/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test SQL Server-specific query support aspects.
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

    {
      object o1;
      object o2;
      object o3;

      o1.num = 1;
      o1.str = "aaa";
      o1.nstr = L"aaa";
      o1.lstr.assign (1024, 'a');
      o1.lnstr.assign (1024, L'a');
      o1.smoney = 11000;
      o1.money = 1.1;

      o2.num = 2;
      o2.str = "bbb";
      o2.nstr = L"bbb";
      o2.lstr.assign (1024, 'b');
      o2.lnstr.assign (1024, L'b');
      o2.smoney = 22000;
      o2.money = 2.2;

      o3.num = 3;
      o3.str = "ccc";
      o3.nstr = L"ccc";
      o3.lstr.assign (1024, 'c');
      o3.lnstr.assign (1024, L'c');
      o3.smoney = 33000;
      o3.money = 3.3;

      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      db->persist (o3);
      t.commit ();
    }

    typedef mssql::query<object> query;
    typedef odb::result<object> result;

    {
      transaction t (db->begin ());

      // Money and small money.
      //
      {
        result r (db->query<object> (query::smoney < 22000));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->smoney == 11000);
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> ("smoney < " + query::_val (2.1F)));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->smoney == 11000);
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> (query::money < 2.2));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->money == 1.1);
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> ("money < " + query::_val (2.2)));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->money == 1.1);
        assert (++i == r.end ());
      }

      // Short/long string.
      //
      {
        result r (db->query<object> (query::str < "bbb"));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->str == "aaa");
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> ("str < " + query::_val ("bbb", 3)));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->str == "aaa");
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> (query::nstr < L"bbb"));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->nstr == L"aaa");
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> (query::lstr < string (1024, 'b')));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->lstr == string (1024, 'a'));
        assert (++i == r.end ());
      }

      {
        string v (1024, 'b');
        result r (db->query<object> (query::lstr < query::_ref (v)));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->lstr == string (1024, 'a'));
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> (query::lnstr < wstring (1024, L'b')));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->lnstr == wstring (1024, L'a'));
        assert (++i == r.end ());
      }

      // Test image copying with long data.
      //
      {
        result r (db->query<object> (query::str < "ccc"));
        result::iterator i (r.begin ());

        assert (i != r.end ());
        ++i;
        assert (i != r.end ());

        {
          result r (db->query<object> (query::str < "bbb"));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->str == "aaa");
          assert (++i == r.end ());
        }

        assert (i->str == "bbb"); // Load from copy.
        assert (++i == r.end ());
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
