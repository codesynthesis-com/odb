// file      : oracle/types/driver.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Oracle type conversion.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

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
    // Create an Oracle database instance, setting both the client database
    // and national character set to UTF-8.
    //
    auto_ptr<database> db (create_database (argc, argv, false));

    object o (1);

    o.int_ = -123456;
    o.uint_ = 123456;
    o.long_long_ = -123456;
    o.ulong_long_ = 123456;

    o.float_ = 1.123F;
    o.double_ = 1.123;
    o.binary_float_ = 1.123F;
    o.binary_double_ = 1.123;

    o.date_ = date_time (2010, 8, 29, 15, 33, 18);

    string vshort_str (8, 's');
    string short_str (13, 's');
    string medium_str (104, 'm');
    string long_str (1018, 'l');
    string vlong_str (15000, 'v');

    o.char_ = short_str;
    o.varchar2_ = medium_str;
    o.clob_.assign (vlong_str.data (), vlong_str.data () + vlong_str.size ());

    o.nchar_ = vshort_str;
    o.nvarchar2_ = medium_str;
    o.nclob_.assign (vlong_str.data (), vlong_str.data () + vlong_str.size ());

    o.raw_.assign (long_str.data (), long_str.data () + long_str.size ());
    o.blob_.assign (vlong_str.data (), vlong_str.data () + vlong_str.size ());

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

    // Test character set conversion.
    //
    // const char* unicode_str = "a \xD5\x95 \xEA\xAA\xAA \xF2\xAA\xAA\xAA";

    // o.char_ = unicode_str;
    // o.varchar2_ = unicode_str;
    // o.clob_ = unicode_str;

    // o.nchar_ = unicode_str;
    // o.nvarchar2_ = unicode_str;
    // o.nclob_ = unicode_str;

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->update (o);
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

    // Test 64 bit integers.
    //
    big_ints bi (true);
    bi.id = 1;

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (bi);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      auto_ptr<big_ints> bi1 (db->load<big_ints> (1));
      t.commit ();

      assert (bi == *bi1);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
