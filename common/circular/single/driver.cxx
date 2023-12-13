// file      : common/circular/single/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test cases of circular dependencies between persistent classes, single
// file version.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    query<base> bq (query<base>::d->id != 0);
    query<derived> dq (query<derived>::b->id != 0);
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
