// file      : oracle/template/driver.cxx
// copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// PLACE TEST DESCRIPTION HERE
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace oracle = odb::oracle;
using namespace oracle;

int
main (int argc, char* argv[])
{
  try
  {
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
