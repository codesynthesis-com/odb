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
#  include <odb/connection.hxx>
#  include <odb/transaction.hxx>
#  include <odb/schema-catalog.hxx>
#  include <odb/sqlite/database.hxx>
#  include <odb/sqlite/connection-factory.hxx>
#elif defined(DATABASE_PGSQL)
#  include <odb/pgsql/database.hxx>
#  include <odb/pgsql/connection-factory.hxx>
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
#elif defined(DATABASE_PGSQL)
namespace pgsql = odb::pgsql;
#endif

auto_ptr<database>
create_database (int& argc,
                 char* argv[],
#if defined(DATABASE_SQLITE)
                 bool schema,
#else
                 bool,
#endif
                 size_t max_connections)
{
  if (argc > 1 && argv[1] == string ("--help"))
  {
    cerr << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    mysql::database::print_usage (cerr);
#elif defined(DATABASE_SQLITE)
    sqlite::database::print_usage (cerr);
#elif defined(DATABASE_PGSQL)
    pgsql::database::print_usage (cerr);
#endif

    exit (0);
  }

  auto_ptr<database> db;

#if defined(DATABASE_MYSQL)
  auto_ptr<mysql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new mysql::connection_pool_factory (max_connections));

  db.reset (new mysql::database (argc, argv, false, "", 0, f));
#elif defined(DATABASE_SQLITE)
  auto_ptr<sqlite::connection_factory> f;

  if (max_connections != 0)
    f.reset (new sqlite::connection_pool_factory (max_connections));

  db.reset (
    new sqlite::database (
      argc, argv, false, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, true, f));

  // Create the database schema. Due to bugs in SQLite foreign key
  // support for DDL statements, we need to temporarily disable
  // foreign keys.
  //
  if (schema)
  {
    connection_ptr c (db->connection ());

    c->execute ("PRAGMA foreign_keys=OFF");

    transaction t (c->begin ());
    schema_catalog::create_schema (*db);
    t.commit ();

    c->execute ("PRAGMA foreign_keys=ON");
  }
#elif defined(DATABASE_PGSQL)
  auto_ptr<pgsql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new pgsql::connection_pool_factory (max_connections));

  db.reset (new pgsql::database (argc, argv, false, "", f));
#endif

  return db;
}
