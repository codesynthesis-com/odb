// file      : sqlite/database/driver.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test that database constructors are unambiguous (compilation only).
//

#include <odb/sqlite/database.hxx>

using namespace odb::sqlite;

int
main (int argc, char* argv[])
{
  // This code should not execute.
  //
  if (argc != 0)
    return 0;

  {
    database d1 ("db1");
  }

  {
    database d1 (argc, argv);
    database d2 (argc, argv, false);
  }
}
