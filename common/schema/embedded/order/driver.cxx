// file      : common/schema/embedded/order/driver.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test statement execution order in embedded schemas.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <common/config.hxx> // DATABASE_XXX
#include <common/common.hxx>

#include "test1.hxx"
#include "test2.hxx"

#include "test1-odb.hxx"
#include "test2-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv, false));

    // Create the database schema.
    //
    {
      connection_ptr c (db->connection ());

      // Temporarily disable foreign key constraints for SQLite.
      //
#if defined(DATABASE_SQLITE)
      c->execute ("PRAGMA foreign_keys=OFF");
#endif

      transaction t (c->begin ());
      schema_catalog::create_schema (*db);
      t.commit ();

#if defined(DATABASE_SQLITE)
      c->execute ("PRAGMA foreign_keys=ON");
#endif
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
