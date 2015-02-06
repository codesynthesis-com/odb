// file      : boost/sqlite/date-time/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <odb/core.hxx>

#pragma db object
struct object
{
  object ()
  {
  }

  bool
  operator== (const object& x) const
  {
    return
      id == x.id &&
      dates == x.dates &&
      times == x.times &&
      durations == x.durations &&
      u_dates == x.u_dates  &&
      u_times == x.u_times &&
      s_durations == x.s_durations;
  }

  #pragma db id auto
  unsigned long id;

  std::vector<boost::gregorian::date> dates;
  std::vector<boost::posix_time::ptime> times;
  std::vector<boost::posix_time::time_duration> durations;

  // Dates as UNIX time.
  //
  #pragma db value_type("INTEGER")
  std::vector<boost::gregorian::date> u_dates;

  // Times as UNIX time.
  //
  #pragma db value_type("INTEGER")
  std::vector<boost::posix_time::ptime> u_times;

  // Durations as seconds.
  //
  #pragma db value_type("INTEGER")
  std::vector<boost::posix_time::time_duration> s_durations;
};

#endif // TEST_HXX
