// file      : sqlite/types/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test SQLite type conversion.
//

#include <limits>   // std::numeric_limits
#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace sqlite = odb::sqlite;
using namespace sqlite;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    object o (1);

    o.bool_ = true;
    o.integer_ = -123456;
    o.real_ = 1.123;
    o.nan_ = numeric_limits<double>::quiet_NaN ();

    string long_str (2040, 'l');

    o.text_ = long_str;
#ifdef _WIN32
    o.wtext_ = L"t\x00C8st string";
#endif
    o.blob_.assign (long_str.c_str (), long_str.c_str () + long_str.size ());

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o == *o1);
    }

#ifdef _WIN32
    {
      typedef sqlite::query<object> query;
      typedef odb::result<object> result;

      transaction t (db->begin ());
      result r (db->query<object> (query::wtext == L"t\x00C8st string"));
      assert (!r.empty ());
      t.commit ();
    }
#endif

    // Test char/wchar_t arrays
    //
    {
#ifndef _WIN32
      char_array o1 (1, "");
      char_array o2 (2, "1234567890");
      char_array o3 (3, "12345678901234567");
#else
      char_array o1 (1, "", L"");
      char_array o2 (2, "1234567890", L"123456789\x00C8");
      char_array o3 (3, "12345678901234567", L"1234567890123456\x00C8");
#endif

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<char_array> p1 (db->load<char_array> (1));
        auto_ptr<char_array> p2 (db->load<char_array> (2));
        auto_ptr<char_array> p3 (db->load<char_array> (3));
        t.commit ();

        assert (o1 == *p1);
        assert (o2 == *p2);
        assert (o3 == *p3);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
