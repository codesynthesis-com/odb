// file      : qt/mysql/core/driver.cxx
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

    person p1;
    p1.name = "Constantin Michael";
    p1.date_of_birth.setDate (1979, 03, 07);

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (p1);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      person* pl = db->load<person> (p1.name);
      t.commit ();

      assert (*pl == p1);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
