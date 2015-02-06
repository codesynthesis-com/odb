// file      : libcommon/common/common.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::exit
#include <utility> // std::move
#include <iostream>

#include <odb/database.hxx>

#include <common/config.hxx>
#include <common/common.hxx>

using namespace std;
using namespace odb::core;


// MySQL.
//
#if defined(DATABASE_MYSQL) || defined(DATABASE_COMMON)

#include <odb/mysql/database.hxx>
#include <odb/mysql/connection-factory.hxx>

static auto_ptr<database>
create_mysql_database (int& argc, char* argv[], bool, size_t max_connections)
{
  namespace mysql = odb::mysql;

#ifdef HAVE_CXX11
  unique_ptr<mysql::connection_factory> f;
#else
  auto_ptr<mysql::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new mysql::connection_pool_factory (max_connections));

  return auto_ptr<database> (
    new mysql::database (argc, argv, false, "", 0,
#ifdef HAVE_CXX11
                         move (f)
#else
                         f
#endif
    ));
}
#endif // MySQL


// SQLite.
//
#if defined(DATABASE_SQLITE) || defined(DATABASE_COMMON)

#include <odb/connection.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/sqlite/connection-factory.hxx>

static auto_ptr<database>
create_sqlite_database (int& argc,
                        char* argv[],
                        bool schema,
                        size_t max_connections)
{
  namespace sqlite = odb::sqlite;

#ifdef HAVE_CXX11
  unique_ptr<sqlite::connection_factory> f;
#else
  auto_ptr<sqlite::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new sqlite::connection_pool_factory (max_connections));

  auto_ptr<database> db (
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

  return db;
}
#endif // SQLite


// PostgreSQL.
//
#if defined(DATABASE_PGSQL) || defined(DATABASE_COMMON)

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection-factory.hxx>

static auto_ptr<database>
create_pgsql_database (int& argc, char* argv[], bool, size_t max_connections)
{
  namespace pgsql = odb::pgsql;

#ifdef HAVE_CXX11
  unique_ptr<pgsql::connection_factory> f;
#else
  auto_ptr<pgsql::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new pgsql::connection_pool_factory (max_connections));

  return auto_ptr<database> (
    new pgsql::database (argc, argv, false, "",
#ifdef HAVE_CXX11
                         move (f)
#else
                         f
#endif
    ));
}
#endif // PostgreSQL


// Oracle.
//
#if defined(DATABASE_ORACLE) || defined(DATABASE_COMMON)

#include <odb/oracle/database.hxx>
#include <odb/oracle/connection-factory.hxx>

static auto_ptr<database>
create_oracle_database (int& argc, char* argv[], bool, size_t max_connections)
{
  namespace oracle = odb::oracle;

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
  return auto_ptr<database> (
    new oracle::database (argc, argv, false, 873, 873, 0,
#ifdef HAVE_CXX11
                          move (f)
#else
                          f
#endif
    ));
}
#endif // Oracle

// SQL Server.
//
#if defined(DATABASE_MSSQL) || defined(DATABASE_COMMON)

#include <odb/mssql/database.hxx>
#include <odb/mssql/connection-factory.hxx>

static auto_ptr<database>
create_mssql_database (int& argc, char* argv[], bool, size_t max_connections)
{
  namespace mssql = odb::mssql;

#ifdef HAVE_CXX11
  unique_ptr<mssql::connection_factory> f;
#else
  auto_ptr<mssql::connection_factory> f;
#endif

  if (max_connections != 0)
    f.reset (new mssql::connection_pool_factory (max_connections));

  return auto_ptr<database> (
    new mssql::database (argc, argv, false, "",
                         mssql::isolation_read_committed, 0,

#ifdef HAVE_CXX11
                         move (f)
#else
                         f
#endif
    ));
}
#endif // SQL Server

//
//
auto_ptr<database>
create_database (int argc,
                 char* argv[],
                 bool schema,
                 size_t max_connections,
#if defined(DATABASE_COMMON)
                 odb::database_id db
#else
                 odb::database_id
#endif
)
{
  char** argp = argv + 1; // Position of the next argument. Assignment for VC8.
  int argn (argc - 1);    // Number of arguments left.

#if defined(DATABASE_COMMON)
  // Figure out which database we are creating. We may be given the
  // database name as a program argument or as an id.
  //
  if (db == odb::id_common && argn != 0)
  {
    string s (*argp);

    if (s == "mysql")
      db = odb::id_mysql;
    else if (s == "sqlite")
      db = odb::id_sqlite;
    else if (s == "pgsql")
      db = odb::id_pgsql;
    else if (s == "oracle")
      db = odb::id_oracle;
    else if (s == "mssql")
      db = odb::id_mssql;

    if (db != odb::id_common)
    {
      argp++;
      argn--;
    }
  }

  if (db == odb::id_common)
  {
    cerr << "Usage: " << argv[0] << " <db> [options]" << endl;
    exit (1);
  }
#endif

  if (argn != 0 && *argp == string ("--help"))
  {
#if defined(DATABASE_COMMON)
    cout << "Usage: " << argv[0] << " <db> [options]" << endl;
#else
    cout << "Usage: " << argv[0] << " [options]" << endl;
#endif

    cout << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    odb::mysql::database::print_usage (cout);
#elif defined(DATABASE_SQLITE)
    odb::sqlite::database::print_usage (cout);
#elif defined(DATABASE_PGSQL)
    odb::pgsql::database::print_usage (cout);
#elif defined(DATABASE_ORACLE)
    odb::oracle::database::print_usage (cout);
#elif defined(DATABASE_MSSQL)
    odb::mssql::database::print_usage (cout);
#elif defined(DATABASE_COMMON)
    switch (db)
    {
    case odb::id_mysql:
      odb::mysql::database::print_usage (cout);
      break;
    case odb::id_sqlite:
      odb::sqlite::database::print_usage (cout);
      break;
    case odb::id_pgsql:
      odb::pgsql::database::print_usage (cout);
      break;
    case odb::id_oracle:
      odb::oracle::database::print_usage (cout);
      break;
    case odb::id_mssql:
      odb::mssql::database::print_usage (cout);
      break;
    case odb::id_common:
      assert (false);
    }
#else
#  error unknown database
#endif

    exit (0);
  }

#if defined(DATABASE_MYSQL)
  return create_mysql_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_SQLITE)
  return create_sqlite_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_PGSQL)
  return create_pgsql_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_ORACLE)
  return create_oracle_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_MSSQL)
  return create_mssql_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_COMMON)
  switch (db)
  {
  case odb::id_mysql:
    return create_mysql_database (argc, argv, schema, max_connections);
  case odb::id_sqlite:
    return create_sqlite_database (argc, argv, schema, max_connections);
  case odb::id_pgsql:
    return create_pgsql_database (argc, argv, schema, max_connections);
  case odb::id_oracle:
    return create_oracle_database (argc, argv, schema, max_connections);
  case odb::id_mssql:
    return create_mssql_database (argc, argv, schema, max_connections);
  case odb::id_common:
    assert (false);
  }
  return auto_ptr<database> ();
#else
#  error unknown database
#endif
}

bool
size_available ()
{
#if defined(DATABASE_SQLITE) || \
    defined(DATABASE_ORACLE) || \
    defined(DATABASE_MSSQL)  || \
    defined(DATABASE_COMMON)
  return false;
#else
  return true;
#endif
}
