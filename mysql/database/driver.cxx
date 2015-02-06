// file      : mysql/database/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test that database constructors are unambiguous (compilation only).
//

#include <string>

#include <odb/mysql/database.hxx>

using std::string;
namespace mysql = odb::mysql;
using namespace mysql;

int
main (int argc, char* argv[])
{
  // This code should not execute.
  //
  if (argc != 0)
    return 0;

  {
    database d1 (0, 0, 0);
    database d2 ("bob", "secret", "db1");
    database d3 ("bob", "secret", "db1", "server1");
    database d4 ("bob", "secret", "db1", "server1", 999);
    database d5 ("bob", "secret", "db1", "server1", 999, "sock1");
    database d6 ("bob", "secret", "db1", "server1", 999, "sock1", "charset1");
  }

  std::string u ("bob"), p ("secret"), db ("bd1"), h ("server1"),
    s ("sock1"), cs ("charset1");

  {
    database d1 (u, p, db);
    database d2 (u, p, db, h);
    database d3 (u, p, db, h, 999);
    database d4 (u, p, db, h, 999, &s);
    database d5 (u, p, db, h, 999, &s, cs);
  }

  {
    database d1 (u, 0, db);
    database d2 (u, &p, db);
    database d3 (u, &p, db, h);
    database d4 (u, &p, db, h, 999);
    database d5 (u, &p, db, h, 999, &s);
    database d6 (u, &p, db, h, 999, &s, cs);
  }

  {
    database d1 (u, p, db, h, 999, "socket1");
    database d2 (u, p, db, h, 999, s);
    database d3 (u, p, db, h, 999, s, cs);
  }

  {
    database d1 (u, 0, db, h, 999, s);
    database d2 (u, &p, db, h, 999, "socket1");
    database d3 (u, &p, db, h, 999, s, cs);
  }

  {
    database d1 (argc, argv);
    database d2 (argc, argv, false);
    database d3 (argc, argv, true, "charset1");
  }
}
