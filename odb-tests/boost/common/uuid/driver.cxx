// file      : boost/common/uuid/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Boost UUID persistence.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <boost/uuid/uuid_generators.hpp>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace boost::uuids;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    object o (1);
    o.uuid_ = random_generator() ();
    o.null_ = nil_uuid ();
    o.zero_ = nil_uuid ();

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      unique_ptr<object> p (db->load<object> (o.id_));
      t.commit ();

      assert (*p == o);
    }

    {
      typedef odb::query<object> query;
      typedef odb::result<object> result;

      transaction t (db->begin ());
      result r (db->query<object> (query::uuid == o.uuid_));
      result::iterator i (r.begin ());
      assert (i != r.end () && i->id_ == o.id_);
      assert (++i == r.end ());
      t.commit ();
    }

  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
