// file      : tracer/template/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// PLACE TEST DESCRIPTION HERE
//

#include <memory>
#include <cassert>
#include <iostream>

#include <odb/exceptions.hxx>
#include <odb/transaction.hxx>
#include <odb/tracer/database.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb;

int
main ()
{
  tracer::database db;

  //
  //
  cout << "test 001" << endl;
  {
    transaction t (db.begin ());
    t.commit ();
  }
}
