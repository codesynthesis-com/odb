// file      : sqlite/native/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test native SQL execution.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <common/common.hxx>

using namespace std;
namespace sqlite = odb::sqlite;
using namespace sqlite;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (
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
