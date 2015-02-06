// file      : pgsql/database/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test that database constructors are unambiguous (compilation only).
//

#include <odb/pgsql/database.hxx>

namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  // This code should not execute.
  //
  if (argc != 0)
    return 0;

  {
    database d1 ("bob", "secret", "db1");
    database d2 ("bob", "secret", "db1", "server1");
    database d3 ("bob", "secret", "db1", "server1", 999);
    database d4 ("bob", "secret", "db1", "server1", 999, "extra");
  }

  {
    database d1 ("bob", "secret", "db1", "server1", "ext1");
    database d2 ("bob", "secret", "db1", "server1", "ext1", "extra");
  }

  {
    database d1 ("conninfo");
  }

  {
    database d1 (argc, argv);
    database d2 (argc, argv, false);
    database d3 (argc, argv, true, "extra");
  }
}
