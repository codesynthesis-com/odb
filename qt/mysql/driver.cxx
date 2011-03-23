// file      : qt/mysql/driver.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt core type persistence. MySQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    person p;
    p.name = "John Doe";

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (p);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      person* pl = db->load<person> (p.id);
      t.commit ();

      assert (*pl == p);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
