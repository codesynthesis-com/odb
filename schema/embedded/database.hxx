// file      : schema/embedded/database.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : not copyrighted - public domain

//
// Create concrete database instance based on the DATABASE_* macros.
//

#ifndef DATABASE_HXX
#define DATABASE_HXX

#include <string>
#include <memory>   // std::auto_ptr
#include <cstdlib>  // std::exit
#include <iostream>

#include <odb/database.hxx>

#if defined(DATABASE_MYSQL)
#  include <odb/mysql/database.hxx>
#elif defined(DATABASE_SQLITE)
#  include <odb/sqlite/database.hxx>
#elif defined(DATABASE_PGSQL)
#  include <odb/pgsql/database.hxx>
#elif defined(DATABASE_ORACLE)
#  include <odb/oracle/database.hxx>
#endif

inline std::auto_ptr<odb::database>
create_database (int& argc, char* argv[])
{
  using namespace std;
  using namespace odb::core;

  if (argc > 1 && argv[1] == string ("--help"))
  {
    cerr << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    odb::mysql::database::print_usage (cerr);
#elif defined(DATABASE_SQLITE)
    odb::sqlite::database::print_usage (cerr);
#elif defined(DATABASE_PGSQL)
    odb::pgsql::database::print_usage (cerr);
#elif defined(DATABASE_ORACLE)
    odb::oracle::database::print_usage (cerr);
#endif

    exit (0);
  }

#if defined(DATABASE_MYSQL)
  auto_ptr<database> db (new odb::mysql::database (argc, argv));
#elif defined(DATABASE_SQLITE)
  auto_ptr<database> db (
    new odb::sqlite::database (
      argc, argv, false, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
#elif defined(DATABASE_PGSQL)
  auto_ptr<database> db (new odb::pgsql::database (argc, argv));
#elif defined(DATABASE_ORACLE)
  auto_ptr<database> db (
    new odb::oracle::database (argc, argv));
#endif

  return db;
}

#endif // DATABASE_HXX
