// file      : common/schema/embedded/basics/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test basic embedded schema functionality.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

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
    unique_ptr<database> db (create_database (argc, argv, false));

    // Create the database schema.
    //
    {
      assert (schema_catalog::exists (*db, "test"));
      assert (!schema_catalog::exists (*db, "test1"));
      assert (!schema_catalog::exists (*db, ""));

      transaction t (db->begin ());
      schema_catalog::create_schema (*db, "test");
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
