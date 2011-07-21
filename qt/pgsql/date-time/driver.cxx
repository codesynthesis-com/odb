// file      : qt/pgsql/date-time/driver.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. PostgreSQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QDateTime>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

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
      o2.time = ct_no_ms.time ();
      o2.date_time = ct_no_ms;

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

    // Test out of range values for QDateTime traits.
    //
    {
      object o;
      o.date_time = QDateTime (QDate (1969, 12, 31),
                               QTime (23, 59, 59),
                               Qt::UTC);

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
