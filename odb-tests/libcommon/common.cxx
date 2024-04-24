// file      : libcommon/common.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::exit
#include <utility> // std::move
#include <iostream>

#include <odb/database.hxx>

#include <libcommon/config.hxx>
#include <libcommon/common.hxx>

// MySQL.
//
#if defined(DATABASE_MYSQL)

#include <odb/mysql/database.hxx>
#include <odb/mysql/connection-factory.hxx>

static std::unique_ptr<odb::database>
create_mysql_database (int& argc, char* argv[], bool, size_t max_connections)
{
  using namespace std;
  using namespace odb::core;
  namespace mysql = odb::mysql;

  unique_ptr<mysql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new mysql::connection_pool_factory (max_connections));

  return unique_ptr<database> (
    new mysql::database (argc, argv, false, "", 0, move (f)));
}
#endif // MySQL


// SQLite.
//
#if defined(DATABASE_SQLITE)

#include <odb/connection.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/sqlite/connection-factory.hxx>

static std::unique_ptr<odb::database>
create_sqlite_database (int& argc,
                        char* argv[],
                        bool schema,
                        size_t max_connections)
{
  using namespace std;
  using namespace odb::core;
  namespace sqlite = odb::sqlite;

  unique_ptr<sqlite::connection_factory> f;

  if (max_connections != 0)
    f.reset (new sqlite::connection_pool_factory (max_connections));

  unique_ptr<database> db (
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
      move (f)));

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
#if defined(DATABASE_PGSQL)

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection-factory.hxx>

static std::unique_ptr<odb::database>
create_pgsql_database (int& argc, char* argv[], bool, size_t max_connections)
{
  using namespace std;
  using namespace odb::core;
  namespace pgsql = odb::pgsql;

  unique_ptr<pgsql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new pgsql::connection_pool_factory (max_connections));

  return unique_ptr<database> (
    new pgsql::database (argc, argv, false, "", move (f)));
}
#endif // PostgreSQL


// Oracle.
//
#if defined(DATABASE_ORACLE)

#include <odb/oracle/database.hxx>
#include <odb/oracle/connection-factory.hxx>

static std::unique_ptr<odb::database>
create_oracle_database (int& argc, char* argv[], bool, size_t max_connections)
{
  using namespace std;
  using namespace odb::core;
  namespace oracle = odb::oracle;

  unique_ptr<oracle::connection_factory> f;

  if (max_connections != 0)
    f.reset (new oracle::connection_pool_factory (max_connections));

  // Set client database character set and client national character set
  // to UTF-8.
  //
  return unique_ptr<database> (
    new oracle::database (argc, argv, false, 873, 873, 0, move (f)));
}
#endif // Oracle

// SQL Server.
//
#if defined(DATABASE_MSSQL)

#include <odb/mssql/database.hxx>
#include <odb/mssql/connection-factory.hxx>

static std::unique_ptr<odb::database>
create_mssql_database (int& argc, char* argv[], bool, size_t max_connections)
{
  using namespace std;
  using namespace odb::core;
  namespace mssql = odb::mssql;

  unique_ptr<mssql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new mssql::connection_pool_factory (max_connections));

  return unique_ptr<database> (
    new mssql::database (argc, argv, false, "",
                         mssql::isolation_read_committed, 0, move (f)));
}
#endif // SQL Server

//
//
std::unique_ptr<odb::database>
create_database (int argc,
                 char* argv[],
                 bool schema,
                 size_t max_connections,
#if defined(MULTI_DATABASE)
                 odb::database_id db
#else
                 odb::database_id
#endif
)
{
  using namespace std;
  using namespace odb::core;

  char** argp = argv + 1; // Position of the next argument. Assignment for VC8.
  int argn (argc - 1);    // Number of arguments left.

#if defined(MULTI_DATABASE)
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
#if defined(MULTI_DATABASE)
    cout << "Usage: " << argv[0] << " <db> [options]" << endl;
#else
    cout << "Usage: " << argv[0] << " [options]" << endl;
#endif

    cout << "Options:" << endl;

#if defined(MULTI_DATABASE)
    switch (db)
    {
    case odb::id_mysql:
#if defined(DATABASE_MYSQL)
      odb::mysql::database::print_usage (cout);
#else
    assert (false);
#endif
      break;
    case odb::id_sqlite:
#if defined(DATABASE_SQLITE)
      odb::sqlite::database::print_usage (cout);
#else
    assert (false);
#endif
      break;
    case odb::id_pgsql:
#if defined(DATABASE_PGSQL)
      odb::pgsql::database::print_usage (cout);
#else
    assert (false);
#endif
      break;
    case odb::id_oracle:
#if defined(DATABASE_ORACLE)
      odb::oracle::database::print_usage (cout);
#else
    assert (false);
#endif
      break;
    case odb::id_mssql:
#if defined(DATABASE_MSSQL)
      odb::mssql::database::print_usage (cout);
#else
    assert (false);
#endif
      break;
    case odb::id_common:
      assert (false);
    }
#elif defined(DATABASE_MYSQL)
    odb::mysql::database::print_usage (cout);
#elif defined(DATABASE_SQLITE)
    odb::sqlite::database::print_usage (cout);
#elif defined(DATABASE_PGSQL)
    odb::pgsql::database::print_usage (cout);
#elif defined(DATABASE_ORACLE)
    odb::oracle::database::print_usage (cout);
#elif defined(DATABASE_MSSQL)
    odb::mssql::database::print_usage (cout);
#else
#  error unknown database
#endif

    exit (0);
  }

#if defined(MULTI_DATABASE)
  switch (db)
  {
  case odb::id_mysql:
#if defined(DATABASE_MYSQL)
    return create_mysql_database (argc, argv, schema, max_connections);
#else
    assert (false);
    break;
#endif
  case odb::id_sqlite:
#if defined(DATABASE_SQLITE)
    return create_sqlite_database (argc, argv, schema, max_connections);
#else
    assert (false);
    break;
#endif
  case odb::id_pgsql:
#if defined(DATABASE_PGSQL)
    return create_pgsql_database (argc, argv, schema, max_connections);
#else
    assert (false);
    break;
#endif
  case odb::id_oracle:
#if defined(DATABASE_ORACLE)
    return create_oracle_database (argc, argv, schema, max_connections);
#else
    assert (false);
    break;
#endif
  case odb::id_mssql:
#if defined(DATABASE_MSSQL)
    return create_mssql_database (argc, argv, schema, max_connections);
#else
    assert (false);
    break;
#endif
  case odb::id_common:
    assert (false);
  }
  return unique_ptr<database> ();
#elif defined(DATABASE_MYSQL)
  return create_mysql_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_SQLITE)
  return create_sqlite_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_PGSQL)
  return create_pgsql_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_ORACLE)
  return create_oracle_database (argc, argv, schema, max_connections);
#elif defined(DATABASE_MSSQL)
  return create_mssql_database (argc, argv, schema, max_connections);
#else
#  error unknown database
#endif
}

bool
size_available ()
{
#if defined(MULTI_DATABASE)  || \
    defined(DATABASE_SQLITE) || \
    defined(DATABASE_ORACLE) || \
    defined(DATABASE_MSSQL)
  return false;
#else
  return true;
#endif
}
