// file      : mysql/types/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test MySQL type conversion.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace mysql = odb::mysql;
using namespace mysql;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    mysql_version v;
    {
      transaction t (db->begin ());
      db->query<mysql_version> ().begin ().load (v);
      t.commit ();
    }

    //cerr << "MySQL " << v.major << '.' << v.minor << '.' << v.release
    //     << " protocol " << v.protocol << endl;

    object o (1);

    o.bool_ = true;
    o.schar_ = -123;
    o.uchar_ = 123;
    o.short_ = -12345;
    o.ushort_ = 12345;
    o.mint_ = -123456;
    o.umint_ = 123456;
    o.int_ = -123456;
    o.uint_ = 123456;
    o.long_long_ = -123456;
    o.ulong_long_ = 123456;

    o.float_ = 1.123F;
    o.float8_ = 1.123;
    o.double_ = 1.123;
    o.decimal_ = "123.456";

    o.date_ = date_time (false, 2010, 8, 29, 0, 0, 0);
    o.time_ = date_time (true, 0, 0, 0, 12, 26, 59);
    o.date_time_ = date_time (false, 2010, 8, 29, 12, 26, 59);
    o.timestamp_ = date_time (false, 2010, 8, 29, 12, 26, 59);
    o.year_ = 2010;

    // If we are running against MySQL 5.6.4 or later, add fractional
    // seconds and also alter the table to allow sub-second precision.
    //
    if (v.major > 5 ||
        (v.major == 5 && (v.minor > 6 ||
                          (v.minor == 6 && v.release >= 4))))
    {
      o.time_.microseconds = 123456;
      o.date_time_.microseconds = 234567;
      o.timestamp_.microseconds = 345678;

      transaction t (db->begin ());
      db->execute ("ALTER TABLE `mysql_types_object`"           \
                   "  MODIFY COLUMN `time` TIME(6),"            \
                   "  MODIFY COLUMN `date_time` DATETIME(6),"   \
                   "  MODIFY COLUMN `timestamp` TIMESTAMP(6)");
      t.commit ();
    }

    string short_str (128, 's');
    string medium_str (250, 'm');
    string long_str (2040, 'l');

    const char* sb (short_str.c_str ()), *se (sb + short_str.size ());
    const char* mb (medium_str.c_str ()), *me (mb + medium_str.size ());
    const char* lb (long_str.c_str ()), *le (lb + long_str.size ());

    o.char_ = short_str;
    o.binary_.assign (sb, se);
    o.varchar_ = medium_str;
    o.varbinary_.assign (mb, me);
    o.tinytext_ = short_str;
    o.tinyblob_.assign (sb, se);
    o.text_ = long_str;
    o.blob_.assign (lb, le);
    o.mediumtext_ = long_str;
    o.mediumblob_.assign (lb, le);
    o.longtext_ = long_str;
    o.longblob_.assign (lb, le);

    o.bit_.a = 1;
    o.bit_.b = 0;
    o.bit_.c = 0;
    o.bit_.d = 1;

    o.enum_def_ = green;
    o.enum_cst_ = blue;
    o.enum_str_ = "green";
    o.set_.insert ("green");
    o.set_.insert ("red");
    o.set_.insert ("blue");

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    //
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o == *o1);
    }

    // Test char array.
    //
    {
      char_array o1 (1, "");
      char_array o2 (2, "1234567890");
      char_array o3 (3, "1234567890123456");

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
