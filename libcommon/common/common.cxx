// file      : libcommon/common/common.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::exit
#include <iostream>

#ifndef _MSC_VER
#  include <common/config.h>
#endif

#ifdef DB_ID_MYSQL
#  include <odb/mysql/database.hxx>
#endif

#include <common/common.hxx>
#include <common/options.hxx>

using namespace std;
using namespace odb;

auto_ptr<database>
create_database (int argc, char* argv[])
{
#ifdef DB_ID_MYSQL
  cli::argv_file_scanner scan (argc, argv, "--options-file");
  cli::mysql_options ops (scan);

  if (ops.help ())
  {
    cerr << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;
    cli::mysql_options::print_usage (cerr);
    exit (0);
  }

  return auto_ptr<database> (
    new mysql::database (
      ops.user (),
      ops.passwd_specified () ? &ops.passwd () : 0,
      ops.db_name (),
      ops.host (),
      ops.port (),
      ops.socket_specified () ? &ops.socket () : 0));
#else
  return auto_ptr<database> (0);
#endif
}
