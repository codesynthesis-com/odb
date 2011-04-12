// file      : qt/sqlite/date-time/driver.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. SQLite version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QDateTime>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

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

    // Check persistence of null values.
    //
    object o1;
    {
      transaction t (db->begin ());
      db->persist (o1);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> ol1 (db->load<object> (o1.id));
      t.commit ();

      assert (ol1->is_null ());
    }

    QDateTime ct (QDateTime::currentDateTime ());
    QDateTime ct_no_ms = QDateTime (QDate (ct.date ().year (),
                                           ct.date ().month (),
                                           ct.date ().day ()),
                                    QTime (ct.time ().hour (),
                                           ct.time ().minute (),
                                           ct.time ().second ()));

    // Check persistence of valid dates and times.
    //
    object o2;
    {
      o2.date = ct.date ();
      o2.unix_day = ct.date ();
      o2.time = ct.time ();

      // QTime stored as SQLite INTEGER has second resolution.
      // The millsecond part is zeroed to avoid false negatives.
      //
      o2.seconds = ct_no_ms.time ();

      o2.date_time = ct;

      // UNIX time has second reolution. The millsecond part is
      // zeroed to avoid false negatives.
      //
      o2.unix_time = ct_no_ms;

      transaction t (db->begin ());
      db->persist (o2);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> ol2 (db->load<object> (o2.id));
      t.commit ();

      assert (*ol2 == o2);
    }

    // Test different time zones are handled correctly by
    // QDateTime traits UNIX time implementation.
    //
    object o3, o4, o5;
    {
      // o3 is set to ct local time. o4 is set to ct UTC. o5
      // is set to o3's equivalent UTC time.
      //
      o3.unix_time = ct_no_ms;

      o4.unix_time = ct_no_ms;
      o4.unix_time.setTimeSpec (Qt::UTC);

      o5.unix_time = ct_no_ms.toTimeSpec (Qt::UTC);

      transaction t (db->begin ());
      db->persist (o3);
      db->persist (o4);
      db->persist (o5);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> ol3 (db->load<object> (o3.id));
      auto_ptr<object> ol4 (db->load<object> (o4.id));
      auto_ptr<object> ol5 (db->load<object> (o5.id));
      t.commit ();

      assert (ol3->unix_time != ol4->unix_time);
      assert (ol3->unix_time == ol5->unix_time);
    }

    // Test out of range values for QDateTime traits UNIX time
    // implementation.
    //
    {
      object o;
      o.unix_time = QDateTime (QDate (1969, 12, 31),
                               QTime (23, 59, 59),
                               Qt::UTC);

      assert (test_out_of_range_value (o, *db));
    }

    // Test out of range values for QDate traits UNIX time
    // implementation.
    //
    {
      object o;
      o.unix_day = QDate (QDate (1969, 12, 31));

      assert (test_out_of_range_value (o, *db));
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
    transaction t (db.begin ());
    db.persist (x);
    t.rollback ();

    return false;
  }
  catch (const odb::qt::date_time::value_out_of_range&)
  {
  }

  return true;
}
