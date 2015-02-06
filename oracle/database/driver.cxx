// file      : oracle/database/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test that database constructors are unambiguous (compilation only).
//

#include <odb/oracle/database.hxx>

namespace oracle = odb::oracle;
using namespace oracle;

int
main (int argc, char* argv[])
{
  // This code should not execute.
  //
  if (argc != 0)
    return 0;

  {
    database d1 ("bob", "secret", "db1");
  }

  {
    database d1 ("bob", "secret", "svc1", "server1");
    database d2 ("bob", "secret", "svc1", "server1", 999);
  }

  {
    database d1 (argc, argv);
    database d2 (argc, argv, false);
  }
}
