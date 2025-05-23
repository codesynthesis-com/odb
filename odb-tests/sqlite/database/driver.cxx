// file      : sqlite/database/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test that database constructors are unambiguous and some other things.
//

#include <odb/sqlite/database.hxx>

#undef NDEBUG
#include <cassert>

namespace sqlite = odb::sqlite;
using namespace sqlite;

int
main (int argc, char* argv[])
{
  // Test UTF-16 to UTF-8 conversion.
  //
#ifdef _WIN32
  {
    database d (L"t\x00C8st");
    assert (d.name () == "t\xC3\x88st");
  }
#endif

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
