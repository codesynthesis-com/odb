// file      : boost/pgsql/date-time/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test boost date/time type persistence. PostgreSQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <common/common.hxx>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

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
    o.dates.push_back (day_clock::local_day ());
    o.dates.push_back (date (not_a_date_time));
    o.dates.push_back (date (max_date_time));
    o.dates.push_back (date (min_date_time));

    o.times.push_back (microsec_clock::local_time ());
    o.times.push_back (not_a_date_time);
    o.times.push_back (pos_infin);
    o.times.push_back (neg_infin);
    o.times.push_back (ptime (date (max_date_time),
                              time_duration (16, 23, 0, 123456)));
    o.times.push_back (ptime (date (min_date_time),
                              time_duration (3, 14, 7, 123456)));
    o.times.push_back (ptime (date (1969, 12, 31), // Before PG epoch.
                              time_duration (23, 59, 59, 123000)));

    o.durations.push_back (time_duration (0, 0, 0));
    o.durations.push_back (time_duration (12, 3, 4, 123456));
    o.durations.push_back (time_duration (23, 59, 59, 123456));
    o.durations.push_back (not_a_date_time);

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

    {
      // Test invalid date mappings.
      //
      object sv1, sv2;
      sv1.dates.push_back (date (neg_infin));
      sv2.dates.push_back (date (pos_infin));

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    {
      // Test invalid ptime mappings.
      //
      // object sv1, sv2;
      // sv1.times.push_back (neg_infin);
      // sv2.times.push_back (pos_infin);

      // transaction t (db->begin ());
      // assert (test_invalid_special_value (sv1, db));
      // assert (test_invalid_special_value (sv2, db));
      // t.commit ();
    }

    {
      // Test invalid time_duration mappings.
      //
      // object or1, sv1, sv2;
      // or1.durations.push_back (time_duration (0, 0, -1));
      // sv1.durations.push_back (pos_infin);
      // sv2.durations.push_back (neg_infin);

      // transaction t (db->begin ());
      // assert (test_out_of_range_value (or1, db));
      // assert (test_invalid_special_value (sv1, db));
      // assert (test_invalid_special_value (sv2, db));
      // t.commit ();
    }
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

bool
test_out_of_range_value (object& x, auto_ptr<database>& db)
{
  try
  {
    db->persist (x);
    return false;
  }
  catch (const odb::boost::date_time::value_out_of_range&)
  {
  }

  return true;
}
