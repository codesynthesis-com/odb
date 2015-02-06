// file      : common/transaction/basics/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test basic transaction operations.
//

#include <string>
#include <cassert>
#include <iostream>

#include <odb/tracer.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/statement.hxx>
#include <odb/exceptions.hxx>

#include <common/common.hxx>
#include <common/concrete.hxx>

using namespace std;
using namespace odb::core;

struct transaction_tracer: odb::tracer
{
  virtual void
  execute (connection&, const char* s)
  {
    string str (s);

    if (str == "BEGIN")
      cout << "begin transaction" << endl;
    else if (str == "COMMIT")
      cout << "commit transaction" << endl;
    else if (str == "ROLLBACK")
      cout << "rollback transaction" << endl;
  }

  // Override the other version to get rid of a Sun CC warning.
  //
  virtual void
  execute (connection& c, const statement& s)
  {
    execute (c, s.text ());
  }
};

int
main (int argc, char* argv[])
{
  {
    transaction_tracer tracer;
    auto_ptr<database> db (create_database (argc, argv, false));
    db->tracer (tracer);

    assert (!transaction::has_current ());

    // Current and db accessors.
    //
    cout << "test 001" << endl;
    {
      transaction t (db->begin ());
      assert (&t.database () == db.get ());
      assert (transaction::has_current ());
      assert (&transaction::current () == &t);

      transaction::reset_current ();
      assert (!transaction::has_current ());

      transaction t2 (db->begin (), false);
      assert (!transaction::has_current ());

      transaction::current (t2);
      assert (&transaction::current () == &t2);
    }

    // Commit.
    //
    cout << "test 002" << endl;
    {
      transaction t (db->begin ());
      t.commit ();
    }

    // Rollback.
    //
    cout << "test 003" << endl;
    {
      transaction t (db->begin ());
      t.rollback ();
    }

    // Auto rollback.
    //
    cout << "test 004" << endl;
    {
      transaction t (db->begin ());
    }

    // Nested transaction.
    //
    cout << "test 005" << endl;
    {
      transaction t (db->begin ());

      try
      {
        transaction n (db->begin ());
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
      assert (sizeof (odb_db::transaction) == sizeof (transaction));

      odb_db::transaction t (static_cast<odb_db::database&> (*db).begin ());
      odb_db::transaction& r (odb_db::transaction::current ());
      assert (&t == &r);
    }

    // Transaction restart.
    //
    cout << "test 007" << endl;
    {
      transaction t (db->begin ());
      t.commit ();
      t.reset (db->begin ());
      t.commit ();
    }
  }

  // Test early connection release.
  //
  {
    auto_ptr<database> db (create_database (argc, argv, false, 1));
    transaction t1 (db->begin ());
    t1.commit ();
    transaction t2 (db->begin ());
    t2.rollback ();
    transaction t3 (db->begin ());
    t3.commit ();
  }
}
