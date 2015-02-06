// file      : qt/mysql/date-time/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. MySQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

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
  QCoreApplication app (argc, argv);

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    mysql_version v;
    {
      transaction t (db->begin ());
      db->query<mysql_version> ().begin ().load (v);
      t.commit ();
    }

    // If we are running against MySQL 5.6.4 or later alter the tables
    // to allow sub-second precision.
    //
    bool fs (v.major > 5 ||
             (v.major == 5 && (v.minor > 6 ||
                               (v.minor == 6 && v.release >= 4))));
    if (fs)
    {
      transaction t (db->begin ());
      db->execute ("ALTER TABLE `qt_mysql_dt_object`"               \
                   "  MODIFY COLUMN `date_time` DATETIME(3),"       \
                   "  MODIFY COLUMN `timestamp` TIMESTAMP(3) NULL," \
                   "  MODIFY COLUMN `time` TIME(3)");
      t.commit ();
    }

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
    // but with the milliseconds zeroed. MySQL prior to 5.6.4
    // does not support sub-second prevision.
    //
    QDateTime t (QDateTime::currentDateTime ());

    if (!fs)
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
