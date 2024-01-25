// file      : sqlite/native/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test native SQL execution.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <libcommon/common.hxx>

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
    unique_ptr<database> db (
      create_specific_database<database> (argc, argv, false));

    // Create the database schema.
    //
    {
      transaction t (db->begin ());

      db->execute ("DROP TABLE IF EXISTS sqlitex_native_test");
      db->execute ("CREATE TABLE sqlitex_native_test (n INTEGER PRIMARY KEY)");

      t.commit ();
    }

    // Insert a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("INSERT INTO sqlitex_native_test (n) VALUES (1)") == 1);

      assert (
        db->execute ("INSERT INTO sqlitex_native_test (n) VALUES (2)") == 1);

      t.commit ();
    }

    // Select a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("SELECT n FROM sqlitex_native_test WHERE n < 3") == 2);

      assert (
        db->execute ("SELECT n FROM sqlitex_native_test WHERE n > 3") == 0);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
