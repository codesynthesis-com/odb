// file      : mssql/native/driver.cxx
// license   : GNU GPL; see accompanying LICENSE file

// Test native SQL execution.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <libcommon/common.hxx>

#undef NDEBUG
#include <cassert>

using namespace std;
namespace mssql = odb::mssql;
using namespace mssql;

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

      db->execute ("IF OBJECT_ID('mssql_native_test', 'U') IS NOT NULL\n"
                   "  DROP TABLE mssql_native_test");

      db->execute ("CREATE TABLE mssql_native_test (n int)");

      t.commit ();
    }

    // Insert a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("INSERT INTO mssql_native_test (n) VALUES (1)") == 1);

      assert (
        db->execute ("INSERT INTO mssql_native_test (n) VALUES (2)") == 1);

      t.commit ();
    }

    // Select a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("SELECT n FROM mssql_native_test WHERE n < 3") == 2);

      assert (
        db->execute ("SELECT n FROM mssql_native_test WHERE n > 3") == 0);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
