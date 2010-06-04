// file      : tracer/object/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>
#include <iostream>

#include <odb/transaction.hxx>
#include <odb/tracer/database.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb;

using odb::shared_ptr;

int
main ()
{
  tracer::database db;

  // transient -> persistent in transaction
  //
  cout << "test 001" << endl;
  {
    shared_ptr<object1> o1 (new (shared) object1 (1));
    transaction t (db.begin_transaction ());
    cout << "s 1" << endl;
    db.persist (o1);
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }

  // persistent -> transient in transaction
  //
  cout << "test 002" << endl;
  {
    shared_ptr<object1> o1 (new (shared) object1 (1));
    transaction t (db.begin_transaction ());
    cout << "s 1" << endl;
    db.persist (o1);
    cout << "s 2" << endl;
    db.erase (o1);
    cout << "s 3" << endl;
    t.commit ();
    cout << "s 4" << endl;
  }

  // load in transaction
  //
  cout << "test 003" << endl;
  {
    transaction t (db.begin_transaction ());
    cout << "s 1" << endl;
    shared_ptr<object1> o1 (db.load<object1> (1));
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }

  // persistent/clean -> persistent/dirty in transaction
  //
  cout << "test 004" << endl;
  {
    transaction t (db.begin_transaction ());
    cout << "s 1" << endl;
    shared_ptr<object1> o1 (db.load<object1> (1));
    cout << "s 2" << endl;
    db.modified (o1);
    cout << "s 3" << endl;
    t.commit ();
    cout << "s 4" << endl;
  }
}
