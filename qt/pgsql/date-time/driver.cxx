// file      : qt/pgsql/date-time/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. PostgreSQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

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
  QCoreApplication app (argc, argv);

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Check persistence of null values.
    //
    {
      object o;

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
    }

    // Check persistence of valid dates and times.
    //
    {
      QDateTime ct (QDateTime::currentDateTime ());

      object o;
      o.date = ct.date ();
      o.time = ct.time ();
      o.date_time = ct;

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

    // Test a QDateTime value before PG epoch.
    //
    {
      object o;
      o.date_time = QDateTime (QDate (1969, 12, 31), QTime (23, 59, 59, 123));

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
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
