// file      : tracer/transaction/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test transaction operations.
//

#include <cassert>
#include <iostream>

#include <odb/exceptions.hxx>
#include <odb/transaction.hxx>
#include <odb/tracer/database.hxx>
#include <odb/tracer/transaction.hxx>

using namespace std;
using namespace odb::core;
namespace tracer = odb::tracer;

int
main ()
{
  tracer::database db;

  assert (!transaction::has_current ());

  // Current and db accessors.
  //
  cout << "test 001" << endl;
  {
    transaction t (db.begin ());
    assert (&t.database () == &db);
    assert (transaction::has_current ());
    assert (&transaction::current () == &t);
  }

  // Commit.
  //
  cout << "test 002" << endl;
  {
    transaction t (db.begin ());
    t.commit ();
  }

  // Rollback.
  //
  cout << "test 003" << endl;
  {
    transaction t (db.begin ());
    t.rollback ();
  }

  // Auto rollback.
  //
  cout << "test 004" << endl;
  {
    transaction t (db.begin ());
  }

  // Nested transaction.
  //
  cout << "test 005" << endl;
  {
    transaction t (db.begin ());
    try
    {
      transaction n (db.begin ());
    }
    catch (const already_in_transaction&)
    {
      cout << "already_in_transaction" << endl;
    }
  }

  // Concrete transaction type.
  //
  cout << "test 006" << endl;
  {
    assert (sizeof (tracer::transaction) == sizeof (transaction));

    tracer::transaction t (db.begin ());
    tracer::transaction& r (tracer::transaction::current ());
    assert (&t == &r);
  }
}
