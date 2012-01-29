// file      : boost/mysql/date-time/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
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
      timestamps == x.timestamps &&
      durations == x.durations;
  }

  #pragma db id auto
  unsigned long id;

  std::vector<boost::gregorian::date> dates;
  std::vector<boost::posix_time::ptime> times;

  // Make timestamp NULL-able to suppress the auto-initialization and
  // auto-update characteristics of the TIMESTAMP datatype, and to
  // allow NULL values.
  //
  #pragma db value_type ("TIMESTAMP") value_null
  std::vector<boost::posix_time::ptime> timestamps;

  std::vector<boost::posix_time::time_duration> durations;
};

#endif // TEST_HXX
