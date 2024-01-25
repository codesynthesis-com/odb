// file      : common/enum/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test automatic C++ enum mapping.
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
    typedef odb::query<object> query;
    typedef odb::result<object> result;

    unique_ptr<database> db (create_database (argc, argv));

    object o;
    o.color_ = green;
    o.taste_ = object::sweet;
    o.position_ = object::left;

    o.gender_ = object::gender::female;
    o.scale_ = object::scale::ten;
    o.yesno_ = object::yesno::yes;

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      unique_ptr<object> o1 (db->load<object> (o.id_));
      t.commit ();

      assert (o == *o1);
    }

    {
      transaction t (db->begin ());

      result r1 (db->query<object> (query::color == blue));
      result r2 (db->query<object> (query::taste == object::sweet));
      result r3 (db->query<object> (query::position == object::left));

      assert (r1.empty ());
      assert (!r2.empty ());
      assert (!r3.empty ());

      result r4 (db->query<object> (query::gender == object::gender::female));
      result r5 (db->query<object> (query::scale == object::scale::ten));
      result r6 (db->query<object> (query::yesno == object::yesno::yes));

      assert (!r4.empty ());
      assert (!r5.empty ());
      assert (!r6.empty ());

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
