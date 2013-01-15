// file      : libcommon/common/common.cxx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::exit
#include <utility> // std::move
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
#elif defined(DATABASE_ORACLE)
#  include <odb/oracle/database.hxx>
#  include <odb/oracle/connection-factory.hxx>
#elif defined(DATABASE_MSSQL)
#  include <odb/mssql/database.hxx>
#  include <odb/mssql/connection-factory.hxx>
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
#elif defined(DATABASE_ORACLE)
namespace oracle = odb::oracle;
#elif defined(DATABASE_MSSQL)
namespace mssql = odb::mssql;
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
    cout << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    mysql::database::print_usage (cout);
#elif defined(DATABASE_SQLITE)
    sqlite::database::print_usage (cout);
#elif defined(DATABASE_PGSQL)
    pgsql::database::print_usage (cout);
#elif defined(DATABASE_ORAClE)
    oracle::database::print_usage (cout);
#endif

    exit (0);
  }

  auto_ptr<database> db;

#if defined(DATABASE_MYSQL)

#ifdef HAVE_CXX11
  unique_ptr<mysql::connection_factory> f;
#else
  auto_ptr<mysql::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new mysql::connection_pool_factory (max_connections));

  db.reset (new mysql::database (argc, argv, false, "", 0,
#ifdef HAVE_CXX11
                                 move (f)
#else
                                 f
#endif
            ));

#elif defined(DATABASE_SQLITE)

#ifdef HAVE_CXX11
  unique_ptr<sqlite::connection_factory> f;
#else
  auto_ptr<sqlite::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new sqlite::connection_pool_factory (max_connections));

  db.reset (
    new sqlite::database (
      argc, argv, false,
      SQLITE_OPEN_READWRITE
      | SQLITE_OPEN_CREATE
#ifdef SQLITE_OPEN_URI
      | SQLITE_OPEN_URI
#endif
      ,
      true,
      "",
#ifdef HAVE_CXX11
      move (f)
#else
      f
#endif
    ));

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

#ifdef HAVE_CXX11
  unique_ptr<pgsql::connection_factory> f;
#else
  auto_ptr<pgsql::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new pgsql::connection_pool_factory (max_connections));

  db.reset (new pgsql::database (argc, argv, false, "",
#ifdef HAVE_CXX11
                                 move (f)
#else
                                 f
#endif
            ));

#elif defined(DATABASE_ORACLE)

#ifdef HAVE_CXX11
  unique_ptr<oracle::connection_factory> f;
#else
  auto_ptr<oracle::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new oracle::connection_pool_factory (max_connections));

  // Set client database character set and client national character set
  // to UTF-8.
  //
  db.reset (new oracle::database (argc, argv, false, 873, 873, 0,
#ifdef HAVE_CXX11
                                  move (f)
#else
                                  f
#endif
            ));

#elif defined(DATABASE_MSSQL)

#ifdef HAVE_CXX11
  unique_ptr<mssql::connection_factory> f;
#else
  auto_ptr<mssql::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new mssql::connection_pool_factory (max_connections));

  db.reset (new mssql::database (argc, argv, false, "",
                                 mssql::isolation_read_committed, 0,

#ifdef HAVE_CXX11
                                 move (f)
#else
                                 f
#endif
            ));
#endif

  return db;
}

bool
size_available ()
{
#if defined(DATABASE_SQLITE) || \
    defined(DATABASE_ORACLE) || \
    defined(DATABASE_MSSQL)
  return false;
#else
  return true;
#endif
}
