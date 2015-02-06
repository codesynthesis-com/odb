// file      : pgsql/index/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test PostgreSQL index creation. See also the common test.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  try
  {
    // This is just a schema creation test.
    //
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

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
