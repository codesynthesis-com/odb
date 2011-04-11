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
    QDateTime t (QDateTime::currentDateTime ());
    o.date = t.date ();
    o.julian_day = t.date ();
    o.time = t.time ();

    // We do not take milliseconds into account when storing
    // time as seconds. Zero the millisecond part to avoid a
    // false negative.
    //
    o.seconds = QTime (t.time ().hour (),
                       t.time ().minute (),
                       t.time ().second ());
    o.date_time = t;

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
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
