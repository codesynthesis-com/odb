// file      : common/circular/multiple/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test cases of circular dependencies between persistent classes, multiple
// files version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/connection.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <common/common.hxx>

#include "test1.hxx"
#include "test2.hxx"

#include "test2-odb.hxx"
#include "test1-odb.hxx"

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

      // Temporarily disable foreign key constraints for MySQL and SQLite.
      // For these databases this is the only way to drop circularly-
      // dependant tables.
      //
      if (db->id () == odb::id_mysql)
        c->execute ("SET FOREIGN_KEY_CHECKS=0");
      else if (db->id () == odb::id_sqlite)
        c->execute ("PRAGMA foreign_keys=OFF");

      transaction t (c->begin ());
      schema_catalog::create_schema (*db);
      t.commit ();

      if (db->id () == odb::id_mysql)
        c->execute ("SET FOREIGN_KEY_CHECKS=1");
      else if (db->id () == odb::id_sqlite)
        c->execute ("PRAGMA foreign_keys=ON");
    }

    query<base> bq (query<base>::d->id != 0);
    query<derived> dq (query<derived>::b->id != 0);
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
