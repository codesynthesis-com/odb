// file      : boost/common/optional/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test boost::optional persistence.
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

    {
      object o1 (1);
      object o2 (2);
      o2.str = "abc";

      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      unique_ptr<object> o1 (db->load<object> (1));
      unique_ptr<object> o2 (db->load<object> (2));
      t.commit ();

      assert (!o1->str);
      assert (o2->str && *o2->str == "abc");
    }

    {
      typedef odb::query<object> query;
      typedef odb::result<object> result;

      transaction t (db->begin ());

      {
        result r (db->query<object> (query::str.is_null ()));
        assert (!r.empty ());
      }

      {
        result r (db->query<object> (query::str == "abc"));
        assert (!r.empty ());
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
