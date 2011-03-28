// file      : libcommon/common/common.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::exit
#include <iostream>

#include <common/config.hxx>

#include <odb/database.hxx>

#if defined(DATABASE_MYSQL)
#  include <odb/mysql/database.hxx>
#  include <odb/mysql/connection-factory.hxx>
#elif defined(DATABASE_SQLITE)
#  include <odb/transaction.hxx>
#  include <odb/schema-catalog.hxx>
#  include <odb/sqlite/database.hxx>
#  include <odb/sqlite/connection-factory.hxx>
#else
#  error unknown database
#endif

#include <common/common.hxx>

using namespace std;
using namespace odb::core;

#if defined(DATABASE_MYSQL)
namespace mysql = odb::mysql;
#elif defined(DATABASE_SQLITE)
namespace sqlite = odb::sqlite;
#endif

auto_ptr<database>
create_database (int& argc, char* argv[], size_t max_connections)
{
  if (argc > 1 && argv[1] == string ("--help"))
  {
    cerr << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    mysql::database::print_usage (cerr);
#elif defined(DATABASE_SQLITE)
    sqlite::database::print_usage (cerr);
#endif

    exit (0);
  }

  auto_ptr<database> db;

#if defined(DATABASE_MYSQL)
  auto_ptr<mysql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new mysql::connection_pool_factory (max_connections));

  db.reset (new mysql::database (argc, argv, false, 0, f));
#elif defined(DATABASE_SQLITE)
  auto_ptr<sqlite::connection_factory> f;

  if (max_connections != 0)
    f.reset (new sqlite::connection_pool_factory (max_connections));

  db.reset (
    new sqlite::database (
      argc, argv, false, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, f));

  // Create the database schema.
  //
  {
    transaction t (db->begin ());
    schema_catalog::create_schema (*db);
    t.commit ();
  }
#endif

  return db;
}
