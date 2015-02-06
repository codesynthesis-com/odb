// file      : boost/mssql/date-time/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test boost date/time type persistence. SQL Server version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/boost/date-time/exceptions.hxx>

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;

using namespace boost::gregorian;
using namespace boost::posix_time;

using namespace odb::core;

bool
test_invalid_special_value (object&, auto_ptr<database>&);

bool
test_out_of_range_value (object&, auto_ptr<database>&);

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    object o;

    // Test all valid date-time mappings.
    //
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    o.dates.push_back (day_clock::local_day ());
    o.dates.push_back (date (not_a_date_time));
    o.dates.push_back (date (max_date_time));
    o.dates.push_back (date (min_date_time));

    o.times.push_back (second_clock::local_time ());
    o.times.push_back (not_a_date_time);
    o.times.push_back (min_date_time);
    o.times.push_back (ptime (max_date_time));
#endif

    // In DATETIME fractional seconds are rounded to .000, .003, or .007.
    //
    o.times_dt.push_back (ptime (date (2012, 1, 13),
                                 time_duration (11, 57, 13, 7000)));

    // SMALLDATETIME doesn't have seconds (always 0).
    //
    o.times_sdt.push_back (ptime (date (2012, 1, 13),
                                  time_duration (11, 57, 0, 0)));

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    o.durations.push_back (time_duration (1, 2, 3, 123456));
    o.durations.push_back (not_a_date_time);
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

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    // Test invalid date mappings.
    //
    {
      object sv1, sv2;
      sv1.dates.push_back (date (neg_infin));
      sv2.dates.push_back (date (pos_infin));

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    // Test invalid ptime mappings.
    //
    {
      object sv1, sv2;
      sv1.times.push_back (neg_infin);
      sv2.times.push_back (pos_infin);

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    // Test invalid time_duration mappings.
    //
    {
      object sv1, sv2, or1;
      sv1.durations.push_back (pos_infin);
      sv2.durations.push_back (neg_infin);
      or1.durations.push_back (time_duration (50, 2, 3, 123456700));

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));

      try
      {
        db->persist (or1);
        assert (false);
      }
      catch (const odb::boost::date_time::value_out_of_range&)
      {
      }

      t.commit ();
    }
#endif
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}

bool
test_invalid_special_value (object& x, auto_ptr<database>& db)
{
  try
  {
    db->persist (x);
    return false;
  }
  catch (const odb::boost::date_time::special_value&)
  {
  }

  return true;
}
