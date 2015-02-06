// file      : pgsql/types/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
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
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

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

      // PostgreSQL returns padded values for CHAR(N).
      //
      memcpy (o1.s2, "                ", 16);
      o1.s3[0] = o1.c1 = ' ';
      memcpy (o2.s2, "1234567890      ", 16);

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
