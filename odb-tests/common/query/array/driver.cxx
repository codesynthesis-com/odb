// file      : common/query/array/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query support for C arrays.
//

#include <string>
#include <memory>   // std::unique_ptr
#include <cstring>  // std::memcpy
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/config.hxx> // DATABASE_*
#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

#ifndef MULTI_DATABASE
#  if defined(DATABASE_MYSQL)
const odb::mysql::database_type_id bt = odb::mysql::id_blob;
#  elif defined(DATABASE_SQLITE)
const odb::sqlite::database_type_id bt = odb::sqlite::id_blob;
#  elif defined(DATABASE_PGSQL)
const odb::pgsql::database_type_id bt = odb::pgsql::id_bytea;
#  elif defined(DATABASE_ORACLE)
const odb::oracle::database_type_id bt = odb::oracle::id_raw;
#  elif defined(DATABASE_MSSQL)
const odb::mssql::database_type_id bt = odb::mssql::id_binary;
#  else
#    error unknown database
#  endif
#endif

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

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
#ifndef MULTI_DATABASE
      assert (size (db->query<object> (query::s == "abc")) == 1);
      assert (size (db->query<object> (query::s == query::_val ("bcd"))) == 1);
      assert (size (db->query<object> ("s = " + query::_val ("bcd"))) == 1);
      assert (size (db->query<object> ("s = " + query::_ref ("bcd"))) == 1);
#endif

      {
        char a[] = "bcd";
        char* ra = a;
#ifndef MULTI_DATABASE
        assert (size (db->query<object> (query::s == a)) == 1);
        assert (size (db->query<object> (query::s == query::_val (a))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (ra))) == 1);
#ifndef MULTI_DATABASE
        assert (size (db->query<object> ("s = " + query::_val (a))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (a))) == 1);
#endif
      }

      {
        const char a[] = "bcd";
        const char* ra = a;
#ifndef MULTI_DATABASE
        assert (size (db->query<object> (query::s == a)) == 1);
        assert (size (db->query<object> (query::s == query::_val (a))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (ra))) == 1);
#ifndef MULTI_DATABASE
        assert (size (db->query<object> ("s = " + query::_val (a))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (a))) == 1);
#endif
      }

      {
        const char* p = "cde";
#ifndef MULTI_DATABASE
        assert (size (db->query<object> (query::s == p)) == 1);
        assert (size (db->query<object> (query::s == query::_val (p))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (p))) == 1);
#ifndef MULTI_DATABASE
        assert (size (db->query<object> ("s = " + query::_val (p))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (p))) == 1);
#endif
      }

      {
        char a[] = "cde";
        char* p = a;
#ifndef MULTI_DATABASE
        assert (size (db->query<object> (query::s == p)) == 1);
        assert (size (db->query<object> (query::s == query::_val (p))) == 1);
#endif
        assert (size (db->query<object> (query::s == query::_ref (p))) == 1);
#ifndef MULTI_DATABASE
        assert (size (db->query<object> ("s = " + query::_val (p))) == 1);
        assert (size (db->query<object> ("s = " + query::_ref (p))) == 1);
#endif
      }

#ifndef MULTI_DATABASE
      string s ("abc");
      //assert (size (db->query<object> (query::s == s)) == 1);
      assert (size (db->query<object> (query::s == s.c_str ())) == 1);
      //assert (size (db->query<object> (query::s == query::_val (s))) == 1);
      assert (size (db->query<object> (query::s == query::_val (s.c_str ()))) == 1);

      assert (size (db->query<object> ("s = " + query::_val (s))) == 1);
      assert (size (db->query<object> ("s = " + query::_ref (s))) == 1);
#endif

      // @@ BUILD2 Ends up with the following warning, but strangely only in the
      //    multi-database mode:
      //
      // In file included from odb/odb-tests/common/query/array/test-odb.hxx:31,
      //                  from odb/odb-tests/common/query/array/driver.cxx:20:
      // odb/libodb/odb/query-dynamic.hxx: In instantiation of ‘odb::query_base odb::query_column<T>::operator==(const odb::query_column<T2>&) const [with T2 = char [17]; T = char [17]]’:
      // odb/odb-tests/common/query/array/driver.cxx:144:7:   required from here
      // odb/libodb/odb/query-dynamic.hxx:895:43: error: comparison between two arrays is deprecated in C++20 [-Werror=array-compare]
      //   895 |       (void) (sizeof (type_instance<T> () == type_instance<T2> ()));
      //       |                      ~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~
      // odb/libodb/odb/query-dynamic.hxx:895:43: note: use unary ‘+’ which decays operands to pointers or ‘&‘indirect_ref’ not supported by dump_decl<declaration error>[0] == &‘indirect_ref’ not supported by dump_decl<declaration error>[0]’ to compare the addresses
      //
      //    Looks like compile-time assertion. Doesn't make much sense for
      //    arrays since compares pointers to objects rather than objects.
      //    Should we somehow suppress the assertion for arrays or similar?
      //
      //    Note: temporarily ifndef-ed.
      //
#ifndef MULTI_DATABASE
      assert (size (db->query<object> (query::s == query::s1)) == 3);
#endif

      // std::array
      //
      array<char, 17> a;
      memcpy (a.data (), "abc", 4); // VC++ strcpy deprecation.

#ifndef MULTI_DATABASE
      assert (size (db->query<object> (query::a == a)) == 1);
      assert (size (db->query<object> (query::a == query::_val (a))) == 1);
#endif
      assert (size (db->query<object> (query::a == query::_ref (a))) == 1);
#ifndef MULTI_DATABASE
      assert (size (db->query<object> ("a = " + query::_val (a))) == 1);
      assert (size (db->query<object> ("a = " + query::_ref (a))) == 1);
#endif

      // char
      //
      assert (size (db->query<object> (query::c == 'a')) == 1);

      char c ('b');
      assert (size (db->query<object> (query::c == query::_val (c))) == 1);
      assert (size (db->query<object> (query::c == query::_ref (c))) == 1);

#ifndef MULTI_DATABASE
      assert (size (db->query<object> ("c = " + query::_val ('c'))) == 1);
      assert (size (db->query<object> ("c = " + query::_ref (c))) == 1);
#endif

      assert (size (db->query<object> (query::c == query::c1)) == 3);

      // buffer
      //
#ifndef MULTI_DATABASE
      assert (size (db->query<object> (query::b == buf)) == 3);
      assert (size (db->query<object> (query::b == query::_val (buf))) == 3);
#endif

      assert (size (db->query<object> (query::b == query::_ref (buf))) == 3);

#ifndef MULTI_DATABASE
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
