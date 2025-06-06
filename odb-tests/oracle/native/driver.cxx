// file      : oracle/native/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test native SQL execution.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

#include <libcommon/common.hxx>

#undef NDEBUG
#include <cassert>

using namespace std;
namespace oracle = odb::oracle;
using namespace oracle;

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

      db->execute ("BEGIN "
                   "  EXECUTE IMMEDIATE 'DROP TABLE oracle_native_test'; "
                   "  EXCEPTION "
                   "    WHEN OTHERS THEN "
                   "      IF SQLCODE != -942 THEN RAISE; END IF; "
                   "END;");

      db->execute ("CREATE TABLE oracle_native_test (n NUMERIC(10,0))");

      t.commit ();
    }

    // Insert a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("INSERT INTO oracle_native_test (n) VALUES (1)") == 1);

      assert (
        db->execute ("INSERT INTO oracle_native_test (n) VALUES (2)") == 1);

      t.commit ();
    }

    // Select a few rows.
    //
    {
      transaction t (db->begin ());

      assert (
        db->execute ("SELECT n FROM oracle_native_test WHERE n < 3") == 2);

      assert (
        db->execute ("SELECT n FROM oracle_native_test WHERE n > 3") == 0);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
