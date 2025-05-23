// file      : pgsql/native/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test PostgreSQL native SQL execution.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <libcommon/common.hxx>

#undef NDEBUG
#include <cassert>

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_specific_database<database> (argc, argv));

    // Create the database schema.
    //
    {
      transaction t (db->begin ());

      db->execute ("DROP TABLE IF EXISTS pgsql_native_test");
      db->execute ("CREATE TABLE pgsql_native_test (n INT PRIMARY KEY)");

      t.commit ();
    }

    // Insert a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("INSERT INTO pgsql_native_test (n) VALUES (1)") == 1);

      assert (
        db->execute ("INSERT INTO pgsql_native_test (n) VALUES (2)") == 1);

      t.commit ();
    }

    // select a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("SELECT n FROM pgsql_native_test WHERE n < 3") == 2);

      assert (
        db->execute ("SELECT n FROM pgsql_native_test WHERE n > 3") == 0);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }

  return 0;
}
