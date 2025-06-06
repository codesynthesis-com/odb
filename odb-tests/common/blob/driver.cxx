// file      : common/blob/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test BLOB mapping.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    const char data[] =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
      "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"
      "cccccccccccccccccccccccccccccccccccccccccccccccc"
      "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"
      "dddddddddddddddddddddddddddddddddddddddddddddddd"
      "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"
      "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
      "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"
      "ffffffffffffffffffffffffffffffffffffffffffffffff";

    const unsigned char* udata = reinterpret_cast<const unsigned char*> (data);

    object o (1);
    o.vc.assign (data, data + sizeof (data));
    o.vuc.assign (udata, udata + sizeof (data));
    memcpy (o.c, data, sizeof (data));
    memcpy (o.uc, udata, sizeof (data));
    memcpy (o.a.data (), data, sizeof (data));
    memcpy (o.ua.data (), udata, sizeof (data));
    o.cont.push_back (1);
    o.cont.push_back (2);
    o.cont.push_back (3);

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      unique_ptr<object> o1 (db->load<object> (1));
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
