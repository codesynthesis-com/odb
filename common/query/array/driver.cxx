// file      : common/query/array/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query support for C arrays.
//

#include <string>
#include <memory>   // std::auto_ptr
#include <cstring>  // std::memcpy
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/config.hxx> // DATABASE_*
#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

#if defined(DATABASE_MYSQL)
const odb::mysql::database_type_id bt = odb::mysql::id_blob;
#elif defined(DATABASE_SQLITE)
const odb::sqlite::database_type_id bt = odb::sqlite::id_blob;
#elif defined(DATABASE_PGSQL)
const odb::pgsql::database_type_id bt = odb::pgsql::id_bytea;
#elif defined(DATABASE_ORACLE)
const odb::oracle::database_type_id bt = odb::oracle::id_raw;
#elif defined(DATABASE_MSSQL)
const odb::mssql::database_type_id bt = odb::mssql::id_binary;
#elif defined(DATABASE_COMMON)
#else
#  error unknown database
#endif

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    typedef odb::query<object> query;

    const char buf[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6};

    //
    //
    {
      object o1 (1, "abc", buf);
      object o2 (2, "bcd", buf);
      object o3 (3, "cde", buf);

      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      db->persist (o3);
      t.commit ();
    }

    {
      transaction t (db->begin ());

      // string
      //
#ifndef DATABASE_COMMON
      assert (size (db->query<object> (query::s == "abc")) == 1);
      assert (size (db->query<object> (query::s == query::_val ("bcd"))) == 1);
      assert (size (db->query<object> ("s = " + query::_val ("bcd"))) == 1);
      assert (size (db->query<object> ("s = " + query::_ref ("bcd"))) == 1);
#endif

      {
        char a[] = "bcd";
        char* ra = a;
#ifndef DATABASE_COMMON
        assert (size (db->query<object> (query::s == a)) == 1);
        assert (size (db->query<object> (query::s == query::_val (a))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (ra))) == 1);
#ifndef DATABASE_COMMON
        assert (size (db->query<object> ("s = " + query::_val (a))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (a))) == 1);
#endif
      }

      {
        const char a[] = "bcd";
        const char* ra = a;
#ifndef DATABASE_COMMON
        assert (size (db->query<object> (query::s == a)) == 1);
        assert (size (db->query<object> (query::s == query::_val (a))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (ra))) == 1);
#ifndef DATABASE_COMMON
        assert (size (db->query<object> ("s = " + query::_val (a))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (a))) == 1);
#endif
      }

      {
        const char* p = "cde";
#ifndef DATABASE_COMMON
        assert (size (db->query<object> (query::s == p)) == 1);
        assert (size (db->query<object> (query::s == query::_val (p))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (p))) == 1);
#ifndef DATABASE_COMMON
        assert (size (db->query<object> ("s = " + query::_val (p))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (p))) == 1);
#endif
      }

      {
        char a[] = "cde";
        char* p = a;
#ifndef DATABASE_COMMON
        assert (size (db->query<object> (query::s == p)) == 1);
        assert (size (db->query<object> (query::s == query::_val (p))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (p))) == 1);
#ifndef DATABASE_COMMON
        assert (size (db->query<object> ("s = " + query::_val (p))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (p))) == 1);
#endif
      }

#ifndef DATABASE_COMMON
      string s ("abc");
      //assert (size (db->query<object> (query::s == s)) == 1);
      assert (size (db->query<object> (query::s == s.c_str ())) == 1);
      //assert (size (db->query<object> (query::s == query::_val (s))) == 1);
      assert (size (db->query<object> (query::s == query::_val (s.c_str ()))) == 1);

      assert (size (db->query<object> ("s = " + query::_val (s))) == 1);
      assert (size (db->query<object> ("s = " + query::_ref (s))) == 1);
#endif

      assert (size (db->query<object> (query::s == query::s1)) == 3);

      // std::array
      //
#ifdef ODB_CXX11
      array<char, 17> a;
      memcpy (a.data (), "abc", 4); // VC++ strcpy deprecation.

#ifndef DATABASE_COMMON
      assert (size (db->query<object> (query::a == a)) == 1);
      assert (size (db->query<object> (query::a == query::_val (a))) == 1);
#endif
      assert (size (db->query<object> (query::a == query::_ref (a))) == 1);
#ifndef DATABASE_COMMON
      assert (size (db->query<object> ("a = " + query::_val (a))) == 1);
      assert (size (db->query<object> ("a = " + query::_ref (a))) == 1);
#endif
#endif

      // char
      //
      assert (size (db->query<object> (query::c == 'a')) == 1);

      char c ('b');
      assert (size (db->query<object> (query::c == query::_val (c))) == 1);
      assert (size (db->query<object> (query::c == query::_ref (c))) == 1);

#ifndef DATABASE_COMMON
      assert (size (db->query<object> ("c = " + query::_val ('c'))) == 1);
      assert (size (db->query<object> ("c = " + query::_ref (c))) == 1);
#endif

      assert (size (db->query<object> (query::c == query::c1)) == 3);

      // buffer
      //
#ifndef DATABASE_COMMON
      assert (size (db->query<object> (query::b == buf)) == 3);
      assert (size (db->query<object> (query::b == query::_val (buf))) == 3);
#endif

      assert (size (db->query<object> (query::b == query::_ref (buf))) == 3);

#ifndef DATABASE_COMMON
      assert (size (db->query<object> ("b = " + query::_val<bt> (buf))) == 3);
      assert (size (db->query<object> ("b = " + query::_ref<bt> (buf))) == 3);
#endif

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
