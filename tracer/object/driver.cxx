// file      : tracer/object/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test object persistence operations.
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

  // database operation out of transaction
  //
  cout << "\ntest 001" << endl;
  {
    object o1 (1);
    try
    {
      cout << "s 1" << endl;
      db.persist (o1);
      cout << "s 2.a" << endl;
    }
    catch (const not_in_transaction&)
    {
      cout << "s 2.b" << endl;
    }
  }

  // transient -> persistent
  //
  cout << "\ntest 002" << endl;
  {
    object o1 (1);
    transaction t (db.begin ());
    cout << "s 1" << endl;
    db.persist (o1);
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }

  // transient -> persistent (already exist)
  //
  cout << "\ntest 003" << endl;
  {
    object o1 (0);
    transaction t (db.begin ());
    cout << "s 1" << endl;
    try
    {
      db.persist (o1);
    }
    catch (const object_already_persistent&)
    {
      cout << "object already persistent" << endl;
    }
    cout << "s 2" << endl;
  }

  // persistent -> transient
  //
  cout << "\ntest 004" << endl;
  {
    object o1 (1);
    object o2 (2);
    transaction t (db.begin ());
    cout << "s 1" << endl;
    db.persist (o1);
    db.persist (o2);
    cout << "s 2" << endl;
    db.erase (o1);
    db.erase<object> (2);
    cout << "s 3" << endl;
    t.commit ();
    cout << "s 4" << endl;
  }

  // persistent -> transient (not exist)
  //
  cout << "\ntest 005" << endl;
  {
    object o1 (0);
    transaction t (db.begin ());
    cout << "s 1" << endl;
    try
    {
      db.erase (o1);
    }
    catch (const object_not_persistent&)
    {
      cout << "object not persistent" << endl;
    }
    cout << "s 2" << endl;
  }

  // load new object
  //
  cout << "\ntest 006" << endl;
  {
    transaction t (db.begin ());
    cout << "s 1" << endl;
    auto_ptr<object> o1 (db.load<object> (1));
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }

  // load new object (not exist)
  //
  cout << "\ntest 007" << endl;
  {
    transaction t (db.begin ());
    cout << "s 1" << endl;
    try
    {
      auto_ptr<object> o1 (db.load<object> (0));
    }
    catch (const object_not_persistent&)
    {
      cout << "object not persistent" << endl;
    }
    cout << "s 2" << endl;
  }

  // load into existing object
  //
  cout << "\ntest 008" << endl;
  {
    object o1;
    transaction t (db.begin ());
    cout << "s 1" << endl;
    db.load (1, o1);
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }

  // load into existing object (not exist)
  //
  cout << "\ntest 009" << endl;
  {
    object o1;
    transaction t (db.begin ());
    cout << "s 1" << endl;
    try
    {
      db.load (0, o1);
    }
    catch (const object_not_persistent&)
    {
      cout << "object not persistent" << endl;
    }
    cout << "s 2" << endl;
  }

  // update
  //
  cout << "\ntest 010" << endl;
  {
    transaction t (db.begin ());
    cout << "s 1" << endl;
    auto_ptr<object> o1 (db.load<object> (1));
    cout << "s 2" << endl;
    db.update (*o1);
    cout << "s 3" << endl;
    t.commit ();
    cout << "s 4" << endl;
  }

  // update (not exist)
  //
  cout << "\ntest 011" << endl;
  {
    object o1 (0);
    transaction t (db.begin ());
    cout << "s 1" << endl;
    try
    {
      db.update (o1);
    }
    catch (const object_not_persistent&)
    {
      cout << "object not persistent" << endl;
    }
    cout << "s 2" << endl;
  }

  // find new object
  //
  cout << "\ntest 012" << endl;
  {
    transaction t (db.begin ());
    cout << "s 1" << endl;
    auto_ptr<object> o1 (db.find<object> (1));
    assert (o1.get () != 0);
    auto_ptr<object> o2 (db.find<object> (0));
    assert (o2.get () == 0);
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }

  // load into existing object
  //
  cout << "\ntest 013" << endl;
  {
    object o1;
    transaction t (db.begin ());
    cout << "s 1" << endl;
    bool r (db.find (1, o1));
    assert (r);
    r = db.find (0, o1);
    assert (!r);
    cout << "s 2" << endl;
    t.commit ();
    cout << "s 3" << endl;
  }
}
