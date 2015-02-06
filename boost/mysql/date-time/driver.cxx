// file      : boost/mysql/date-time/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test boost date/time type persistence. MySQL version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

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

      db->execute ("ALTER TABLE `boost_mysql_dt_object_durations`"  \
                   "  MODIFY COLUMN `value` TIME(6)");

      db->execute ("ALTER TABLE `boost_mysql_dt_object_times`"      \
                   "  MODIFY COLUMN `value` DATETIME(6)");

      db->execute ("ALTER TABLE `boost_mysql_dt_object_timestamps`" \
                   "  MODIFY COLUMN `value` TIMESTAMP(6) NULL");

      t.commit ();
    }

    object o;

    // Test all valid date-time mappings.
    //
    o.dates.push_back (day_clock::local_day ());
    o.dates.push_back (date (not_a_date_time));
    o.dates.push_back (date (max_date_time));
    o.dates.push_back (date (min_date_time));

    if (fs)
      o.times.push_back (microsec_clock::local_time ());
    else
      o.times.push_back (second_clock::local_time ());
    o.times.push_back (not_a_date_time);
    o.times.push_back (min_date_time);

    // MySQL prior to 5.6.4 does not support fraction seconds. Construct
    // with zero fractional seconds so that comparison test does not fail
    // for invalid reasons.
    //
    o.times.push_back (
      ptime (
        date (max_date_time),
        time_duration (
          ptime (max_date_time).time_of_day ().hours (),
          ptime (max_date_time).time_of_day ().minutes (),
          ptime (max_date_time).time_of_day ().seconds ())));

    if (fs)
      o.timestamps.push_back (microsec_clock::local_time ());
    else
      o.timestamps.push_back (second_clock::local_time ());
    o.timestamps.push_back (not_a_date_time);

    o.durations.push_back (time_duration (1, 2, 3));
    if (fs)
      o.durations.back () += time_duration (microseconds (123456));
    o.durations.push_back (time_duration (-1, 2, 3));
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
      // Test invalid ptime (DATETIME) mappings.
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
      // Test invalid ptime (TIMESTAMP) mappings.
      //
      object or1, or2, sv1, sv2;
      or1.timestamps.push_back (min_date_time);
      or2.timestamps.push_back (max_date_time);
      sv1.timestamps.push_back (neg_infin);
      sv2.timestamps.push_back (pos_infin);

      transaction t (db->begin ());
      assert (test_out_of_range_value (or1, db));
      assert (test_out_of_range_value (or2, db));
      assert (test_invalid_special_value (sv1, db));
      assert (test_invalid_special_value (sv2, db));
      t.commit ();
    }

    {
      // Test invalid time_duration mappings.
      //
      object or1, or2, sv1, sv2;
      or1.durations.push_back (time_duration (850, 0, 0));
      or2.durations.push_back (time_duration (-850, 0, 0));
      sv1.durations.push_back (pos_infin);
      sv2.durations.push_back (neg_infin);

      transaction t (db->begin ());
      assert (test_out_of_range_value (or1, db));
      assert (test_out_of_range_value (or2, db));
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
