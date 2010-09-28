// file      : mysql/types/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
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
using namespace odb;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

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
    o.double_ = 1.123;
    o.decimal_ = "123.456";

    o.date_ = date_time (false, 2010, 8, 29, 0, 0, 0);
    o.time_ = date_time (true, 0, 0, 0, 12, 26, 59);
    o.date_time_ = date_time (false, 2010, 8, 29, 12, 26, 59);
    o.timestamp_ = date_time (false, 2010, 8, 29, 12, 26, 59);
    o.year_ = 2010;

    string short_str (128, 's');
    buffer short_buf (short_str.c_str (), short_str.size ());
    string medium_str (250, 'm');
    buffer medium_buf (medium_str.c_str (), medium_str.size ());
    string long_str (2040, 'l');
    buffer long_buf (long_str.c_str (), long_str.size ());

    o.char_ = short_str;
    o.binary_ = short_buf;
    o.varchar_ = medium_str;
    o.varbinary_ = medium_buf;
    o.tinytext_ = short_str;
    o.tinyblob_ = short_buf;
    o.text_ = long_str;
    o.blob_ = long_buf;
    o.mediumtext_ = long_str;
    o.mediumblob_ = long_buf;
    o.longtext_ = long_str;
    o.longblob_ = long_buf;

    o.bit_.a = 1;
    o.bit_.b = 0;
    o.bit_.c = 0;
    o.bit_.d = 1;

    o.enum_ = "green";
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
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
