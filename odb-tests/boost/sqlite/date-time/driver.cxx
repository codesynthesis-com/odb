// file      : boost/sqlite/date-time/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test boost date/time type persistence. SQLite version.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <libcommon/common.hxx>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;

using namespace boost::gregorian;
using namespace boost::posix_time;

using namespace odb::core;

bool
test_invalid_special_value (object&, unique_ptr<database>&);

bool
test_out_of_range_value (object&, unique_ptr<database>&);

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    object o;

    // Test all valid date-time mappings.
    //
    o.dates.push_back (day_clock::local_day ());
    o.dates.push_back (date (not_a_date_time));
    o.dates.push_back (date (max_date_time));
    o.dates.push_back (date (min_date_time));

    o.times.push_back (second_clock::local_time ());
    o.times.push_back (not_a_date_time);
    o.times.push_back (min_date_time);
    o.times.push_back (max_date_time);

    o.durations.push_back (time_duration (123, 4, 5));
    o.durations.push_back (not_a_date_time);

    o.u_dates.push_back (day_clock::local_day ());
    o.u_dates.push_back (date (not_a_date_time));

    // Boost seems to handle 64 bit std::time_t incorrectly.
    // Insert 32 bit minimum and maximum UNIX time values for now.
    //
    // o.u_dates.push_back (date (max_date_time));
    // o.u_dates.push_back (date (min_date_time));
    //
    o.u_dates.push_back (date (2038, 1, 19));
    o.u_dates.push_back (date (1901, 12, 14));

    o.u_times.push_back (second_clock::local_time ());
    o.u_times.push_back (not_a_date_time);
    o.u_times.push_back (ptime (date (1930, 1, 1), time_duration (0, 0, 0)));

    o.s_durations.push_back (time_duration (123, 4, 5));
    o.s_durations.push_back (time_duration (-12, 3, 4));
    o.s_durations.push_back (not_a_date_time);

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
      object sv1, sv2;
      sv1.times.push_back (neg_infin);
      sv2.times.push_back (pos_infin);

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    {
      // Test invalid time_duration mappings.
      //
      object or1, sv1, sv2;
      or1.durations.push_back (time_duration (0, 0, -1));
      sv1.durations.push_back (pos_infin);
      sv2.durations.push_back (neg_infin);

      transaction t (db->begin ());
      assert (test_out_of_range_value (or1, db));
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    {
      // Test invalid UNIX date mappings.
      //
      object sv1, sv2;
      sv1.u_dates.push_back (date (neg_infin));
      sv2.u_dates.push_back (date (pos_infin));

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    // Test invalid UNIX times mappings.
    //
    {
      object sv1, sv2;
      sv1.u_times.push_back (pos_infin);
      sv2.u_times.push_back (neg_infin);

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    // Test invalid "seconds" duration mappings.
    //
    {
      object sv1, sv2;
      sv1.s_durations.push_back (pos_infin);
      sv2.s_durations.push_back (neg_infin);

      transaction t (db->begin ());
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
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
test_invalid_special_value (object& x, unique_ptr<database>& db)
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
test_out_of_range_value (object& x, unique_ptr<database>& db)
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
