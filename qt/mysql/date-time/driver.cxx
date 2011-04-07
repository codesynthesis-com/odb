// file      : qt/mysql/date-time/driver.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. MySQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QDateTime>

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

bool
test_out_of_range_value (object&, database&);

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    //
    // Check valid dates and times.
    //

    object o;

    // Check persistence of null values.
    //
    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> ol (db->load<object> (o.id));
      t.commit ();

      assert (ol->is_null ());
    }

    // Check persistence of valid dates and times.
    //

    // Create a QDateTime containing the current date and time
    // but with the milliseconds zeroed. MySQL does not currently
    // support millisecond times.
    //
    QDateTime t (QDateTime::currentDateTime ());

    t.setTime (QTime (t.time ().hour (),
                      t.time ().minute (),
                      t.time ().second ()));

    o.date = t.date ();
    o.date_time = t;
    o.timestamp = t;
    o.time = t.time ();

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> ol (db->load<object> (o.id));
      t.commit ();

      assert (*ol == o);
    }

    //
    // Check invalid dates and times.
    //

    {
      // Test out of range dates.
      //
      object or1, or2;
      or1.date = QDate (999, 12, 31);
      or2.date = QDate (10000, 1, 1);

      transaction t (db->begin ());
      assert (test_out_of_range_value (or1, *db));
      assert (test_out_of_range_value (or2, *db));
      t.commit ();
    }

    {
      // Test out of range date-times.
      //
      object or1, or2;
      or1.date_time = QDateTime (QDate (999, 12, 31), QTime (23, 59, 59));
      or2.date_time = QDateTime (QDate (10000, 1, 1));

      transaction t (db->begin ());
      assert (test_out_of_range_value (or1, *db));
      assert (test_out_of_range_value (or2, *db));
      t.commit ();
    }

    {
      // Test out of range timestamps.
      //
      object or1, or2;
      or1.timestamp = QDateTime (QDate (1970, 1, 1));
      or2.timestamp = QDateTime (QDate (2038, 1, 19), QTime (3, 14, 8));

      transaction t (db->begin ());
      assert (test_out_of_range_value (or1, *db));
      assert (test_out_of_range_value (or2, *db));
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}

bool
test_out_of_range_value (object& x, database& db)
{
  try
  {
    db.persist (x);
    return false;
  }
  catch (const odb::qt::date_time::value_out_of_range&)
  {
  }

  return true;
}
