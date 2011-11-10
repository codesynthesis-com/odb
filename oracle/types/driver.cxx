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

    o.date_ = date_time (2010, 8, 29, 15, 33, 18, 0);
    o.timestamp_ = date_time (1996, 3, 9, 18, 2, 54, 123000);
    o.interval_ds_ = time_interval (0, 0, 13, 43, 23, 19, 123000);
    o.interval_ym_ = time_interval (12, 3, 0, 0, 0, 0, 0);

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
    const char* unicode_str = "a \xD5\x95 \xEA\xAA\xAA \xF2\xAA\xAA\xAA";

    // Testing of character set conversion to and from the client's database
    // character set is disabled as the server database character set may
    // not be able to represent some Unicode characters. If this were the case
    // the test outcome would be a false negative.
    //
    // o.char_ = unicode_str;
    // o.varchar2_ = unicode_str;
    // o.clob_ = unicode_str;

    o.nchar_ = unicode_str;
    o.nvarchar2_ = unicode_str;
    o.nclob_ = unicode_str;

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
    big_int bi1 (1, 0x8000000000000000LL);
    big_int bi2 (2, -123456);
    big_int bi3 (3, 0);
    big_int bi4 (4, 123456);
    big_int bi5 (5, 0xFFFFFFFFFFFFFFFFULL);

    big_uint bui1 (1, 0);
    big_uint bui2 (2, 123456);
    big_uint bui3 (3, 0xFFFFFFFFFFFFFFFFULL);

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (bi1);
      db->persist (bi2);
      db->persist (bi3);
      db->persist (bi4);
      db->persist (bi5);
      db->persist (bui1);
      db->persist (bui2);
      db->persist (bui3);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      auto_ptr<big_int> bil1 (db->load<big_int> (1));
      auto_ptr<big_int> bil2 (db->load<big_int> (2));
      auto_ptr<big_int> bil3 (db->load<big_int> (3));
      auto_ptr<big_int> bil4 (db->load<big_int> (4));
      auto_ptr<big_int> bil5 (db->load<big_int> (5));
      auto_ptr<big_uint> buil1 (db->load<big_uint> (1));
      auto_ptr<big_uint> buil2 (db->load<big_uint> (2));
      auto_ptr<big_uint> buil3 (db->load<big_uint> (3));
      t.commit ();

      assert (bi1 == *bil1);
      assert (bi2 == *bil2);
      assert (bi3 == *bil3);
      assert (bi4 == *bil4);
      assert (bi5 == *bil5);
      assert (bui1 == *buil1);
      assert (bui2 == *buil2);
      assert (bui3 == *buil3);
    }

    // Test descriptor management in TIMESTAMP and INTERVAL images.
    //
    {
      typedef odb::query<object> query;
      typedef odb::result<object> result;

      query q (query::timestamp == o.timestamp_ &&
               query::interval_ym == o.interval_ym_ &&
               query::interval_ds == o.interval_ds_);

      transaction t (db->begin ());

      {
        result r (db->query<object> (q));
        assert (size (r) == 1);
      }

      {
        result r (db->query<object> (q));
        assert (size (r) == 1);
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
