// file      : tracer/transaction/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>
#include <iostream>

#include <odb/exceptions.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>
#include <odb/tracer/database.hxx>
#include <odb/tracer/transaction.hxx>

using namespace std;
using namespace odb;

int
main ()
{
  tracer::database db;

  assert (!transaction::has_current ());

  // Current and db accessors.
  //
  cout << "test 001" << endl;
  {
    transaction t (db.begin_transaction ());
    assert (&t.database () == &db);
    assert (transaction::has_current ());
    assert (&transaction::current () == &t);
  }

  // Commit.
  //
  cout << "test 002" << endl;
  {
    transaction t (db.begin_transaction ());
    t.commit ();
  }

  // Rollback.
  //
  cout << "test 003" << endl;
  {
    transaction t (db.begin_transaction ());
    t.rollback ();
  }

  // Auto rollback.
  //
  cout << "test 004" << endl;
  {
    transaction t (db.begin_transaction ());
  }

  // Nested transaction.
  //
  cout << "test 005" << endl;
  {
    transaction t (db.begin_transaction ());
    try
    {
      transaction n (db.begin_transaction ());
    }
    catch (const already_in_transaction&)
    {
      cout << "already_in_transaction" << endl;
    }
  }

  // Existing session via current.
  //
  cout << "test 006" << endl;
  {
    session s;
    transaction t (db.begin_transaction ());
    assert (&t.session () == &s);
  }

  // Existing session passed explicitly.
  //
  cout << "test 007" << endl;
  {
    session s;
    transaction t (db.begin_transaction (s));
    assert (&t.session () == &s);
  }

  // Concrete transaction type.
  //
  cout << "test 008" << endl;
  {
    assert (sizeof (tracer::transaction) == sizeof (transaction));

    tracer::transaction t (db.begin_transaction ());
    tracer::transaction& r (tracer::transaction::current ());
  }
}
