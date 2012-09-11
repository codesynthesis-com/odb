// file      : pgsql/types/driver.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test PostgreSQL type conversion.
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
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    object o (1);

    o.bool_ = true;
    o.short_ = 12345;
    o.int_ = -123456;
    o.long_long_ = 123456;

    o.float_ = 1.123F;
    o.float8_ = 1.123;
    o.double_ = 1.123;

    o.date_ = 4015;
    o.time_ = 48180000000LL;
    o.timestamp_ = 346896000LL;

    string short_str (128, 's');
    string medium_str (250, 'm');
    string long_str (2040, 'l');

    o.char_ = short_str;
    o.varchar_ = medium_str;
    o.text_ = long_str;

    o.bytea_.assign (long_str.c_str (), long_str.c_str () + long_str.size ());

    unsigned char varbit_buf[8] = {1, 3, 1, 3, 1, 3, 1, 3};
    o.varbit_.size = 52;
    o.varbit_.ubuffer_ = ubuffer (varbit_buf, 8);

    o.bit_.a = 0;
    o.bit_.b = 1;
    o.bit_.c = 0;
    o.bit_.d = 1;

    // 6F846D41-C89A-4E4D-B22F-56443CFA543F
    memcpy (o.uuid_, "\x6F\x84\x6D\x41\xC8\x9A\x4E\x4D\xB2\x2F"
            "\x56\x44\x3C\xFA\x54\x3F", 16);

    o.enum_ = green;

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
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
