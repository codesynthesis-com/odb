// file      : qt/oracle/date-time/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt date/time type persistence. Oracle version.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    unique_ptr<database> db (create_database (argc, argv));

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
      unique_ptr<object> ol (db->load<object> (o.id));
      t.commit ();

      assert (ol->is_null ());
    }

    // Check persistence of valid dates and times.
    //
    QDateTime t (QDateTime::currentDateTime ());

    o.date = t.date ();
    o.date_time = t;
    o.date_time_d = QDateTime (QDate (2012, 6, 27), QTime (14, 17, 05, 0));
    o.time = t.time ();

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      unique_ptr<object> ol (db->load<object> (o.id));
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
