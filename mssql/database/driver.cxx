// file      : mssql/database/driver.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL; see accompanying LICENSE file

// Test that database constructors are unambiguous (compilation only).
//

#include <odb/mssql/database.hxx>

namespace mssql = odb::mssql;
using namespace mssql;

int
main (int argc, char* argv[])
{
  // This code should not execute.
  //
  if (argc != 0)
    return 0;

  {
    database d1 ("bob", "secret", "db1", "server1");
    database d2 ("bob", "secret", "db1", "server1", "driver1");
    database d3 ("bob", "secret", "db1", "server1", "driver1", "extra");
  }

  {
    database d1 ("bob", "secret", "db1", protocol_auto);
    database d2 ("bob", "secret", "db1", protocol_auto, "server1");
    database d3 ("bob", "secret", "db1", protocol_auto, "server1", "inst1");
    database d4 ("bob", "secret", "db1", protocol_auto, "server1", "inst1",
                 "driver1");
    database d5 ("bob", "secret", "db1", protocol_auto, "server1", "inst1",
                 "driver1", "extra");
  }

  {
    database d1 ("bob", "secret", "db1", "server1", 0);
    database d2 ("bob", "secret", "db1", "server1", 999, "driver1");
    database d3 ("bob", "secret", "db1", "server1", 999, "driver1", "extra");
  }

  {
    database d1 ("conn1");
  }

  {
    database d1 (argc, argv);
    database d2 (argc, argv, false);
    database d3 (argc, argv, true, "extra");
  }
}
