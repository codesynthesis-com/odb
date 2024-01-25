// file      : sqlite/attach/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test attached database support.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/schema-catalog.hxx>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace sqlite = odb::sqlite;
using namespace sqlite;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> mdb (create_specific_database<database> (argc, argv));

    {
      object o ("one");

      connection_ptr c (mdb->connection ());

      database adb (c, ":memory:", "adb");

      // Create schema similar to create_database().
      //
      {
        c->execute ("PRAGMA foreign_keys=OFF");

        transaction t (c->begin ());
        odb::schema_catalog::create_schema (adb);
        t.commit ();

        c->execute ("PRAGMA foreign_keys=ON");
      }

      {
        transaction t (c->begin ());
        mdb->persist (o);
        adb.persist (o);
        t.commit ();
      }

      {
        transaction t (c->begin ());
        unique_ptr<object> p (adb.load<object> (o.id));
        t.commit ();

        assert (p->str == o.str);
      }

      {
        o.str = "two";

        transaction t (c->begin ());
        adb.update (o);
        t.commit ();
      }

      {
        typedef sqlite::query<object> query;

        transaction t (c->begin ());
        unique_ptr<object> p (adb.query_one<object> (query::str == "two"));
        t.commit ();

        assert (p->str == o.str);
      }

      {
        transaction t (c->begin ());
        adb.erase (o);
        t.commit ();
      }

      {
        transaction t (c->begin ());
        unique_ptr<object> p (mdb->load<object> (o.id));
        t.commit ();

        assert (p.get () != 0);
      }

      adb.detach ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
