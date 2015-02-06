// file      : qt/mssql/date-time/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. SQL Server version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

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
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    QDateTime t (QDateTime::currentDateTime ());

    o.date = t.date ();
    o.date_time = t;
#endif

    // In DATETIME fractional seconds are rounded to .000, .003, or .007.
    //
    o.date_time_dt = QDateTime (QDate (2012, 1, 14),
                                QTime (11, 57, 13, 3));

    // SMALLDATETIME doesn't have seconds (always 0).
    //
    o.date_time_sdt = QDateTime (QDate (2012, 1, 14),
                                 QTime (11, 57, 0, 0));

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    o.time = t.time ();
#endif

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
