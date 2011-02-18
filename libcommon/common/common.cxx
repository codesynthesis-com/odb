// file      : libcommon/common/common.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
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

#if defined(DATABASE_MYSQL)
using namespace odb::mysql;
#endif

auto_ptr<odb::database>
create_database (int& argc, char* argv[], size_t max_connections)
{
  if (argc > 1 && argv[1] == string ("--help"))
  {
    cerr << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

    database::print_usage (cerr);
    exit (0);
  }

#if defined(DATABASE_MYSQL)
  auto_ptr<connection_factory> f;

  if (max_connections != 0)
    f.reset (new connection_pool_factory (max_connections));

  return auto_ptr<odb::database> (new database (argc, argv, false, 0, f));
#endif
}
