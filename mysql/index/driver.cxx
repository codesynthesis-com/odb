// file      : mysql/index/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test MySQL index creation. See also the common test.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace mysql = odb::mysql;
using namespace mysql;

int
main (int argc, char* argv[])
{
  try
  {
    // This is just a schema creation test.
    //
    unique_ptr<database> db (create_specific_database<database> (argc, argv));

    {
      transaction t (db->begin ());
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
