// file      : libcommon/common/common.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::exit
#include <iostream>

#include <common/config.hxx>

#if defined(DATABASE_MYSQL)
#  include <odb/mysql/database.hxx>
#  include <odb/mysql/connection-factory.hxx>
#else
#  error unknown database
#endif

#include <common/common.hxx>

using namespace std;
using namespace odb;

auto_ptr<database>
create_database (int& argc, char* argv[], size_t max_connections)
{
  if (argc > 1 && argv[1] == string ("--help"))
  {
    cerr << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    mysql::database::print_usage (cerr);
#endif

    exit (0);
  }

#if defined(DATABASE_MYSQL)
  auto_ptr<mysql::connection_factory> f;

  if (max_connections != 0)
    f.reset (new mysql::connection_pool_factory (max_connections));

  return auto_ptr<database> (new mysql::database (argc, argv, false, 0, f));
#endif
}
