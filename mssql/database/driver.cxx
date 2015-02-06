// file      : mssql/database/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL; see accompanying LICENSE file

// Test that database constructors are unambiguous (compilation only).
//

#include <string>
#include <cassert>

#include <odb/mssql/database.hxx>

namespace mssql = odb::mssql;
using namespace mssql;

static const char* isolation_map[] = {"1", "2", "3", "5", "4"};

static bool
check_isolation (connection& c, transaction_isolation i)
{
  std::string s ("SELECT 1 FROM sys.dm_exec_sessions WHERE session_id = @@SPID"
                 " AND transaction_isolation_level = ");
  s += isolation_map[i];
  return c.execute (s) == 1;
}

int
main (int argc, char* argv[])
{
  // This code should not execute.
  //
  if (argc == 0)
  {
    {
      database d1 ("bob", "secret", "db1", "server1");
      database d2 ("bob", "secret", "db1", "server1", "driver1");
      database d3 ("bob", "secret", "db1", "server1", "driver1", "extra");
      database d4 ("bob", "secret", "db1", "server1", "driver1", "extra",
                   isolation_read_uncommitted);
    }

    {
      database d1 ("bob", "secret", "db1", protocol_auto);
      database d2 ("bob", "secret", "db1", protocol_auto, "server1");
      database d3 ("bob", "secret", "db1", protocol_auto, "server1", "inst1");
      database d4 ("bob", "secret", "db1", protocol_auto, "server1", "inst1",
                   "driver1");
      database d5 ("bob", "secret", "db1", protocol_auto, "server1", "inst1",
                   "driver1", "extra");
      database d6 ("bob", "secret", "db1", protocol_auto, "server1", "inst1",
                   "driver1", "extra", isolation_read_uncommitted);
    }

    {
      database d1 ("bob", "secret", "db1", "server1", 0);
      database d2 ("bob", "secret", "db1", "server1", 999, "driver1");
      database d3 ("bob", "secret", "db1", "server1", 999, "driver1", "extra");
      database d4 ("bob", "secret", "db1", "server1", 999, "driver1", "extra",
                   isolation_read_uncommitted);
    }

    {
      database d1 ("conn1");
      database d2 ("conn1", isolation_read_uncommitted);
    }

    {
      database d1 (argc, argv);
      database d2 (argc, argv, false);
      database d3 (argc, argv, true, "extra");
      database d4 (argc, argv, false, "extra", isolation_read_uncommitted);
    }
  }

  // Test transaction isolation levels.
  //
  {
    database d (argc, argv, false, "", isolation_read_uncommitted);
    connection_ptr c (d.connection ());
    assert (check_isolation (*c, isolation_read_uncommitted));
  }

  {
    database d (argc, argv, false, "");
    connection_ptr c (d.connection ());
    assert (check_isolation (*c, isolation_read_committed));
  }

  {
    database d (argc, argv, false, "", isolation_repeatable_read);
    connection_ptr c (d.connection ());
    assert (check_isolation (*c, isolation_repeatable_read));
  }

  {
    database d (argc, argv, false, "", isolation_snapshot);
    connection_ptr c (d.connection ());
    assert (check_isolation (*c, isolation_snapshot));
  }

  {
    database d (argc, argv, false, "", isolation_serializable);
    connection_ptr c (d.connection ());
    assert (check_isolation (*c, isolation_serializable));
  }
}
