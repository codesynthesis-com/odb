// file      : boost/mssql/date-time/test.hxx
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

  #pragma db id auto
  unsigned long id;

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  std::vector<boost::gregorian::date> dates;

  std::vector<boost::posix_time::ptime> times;
#endif

  #pragma db value_type("DATETIME")
  std::vector<boost::posix_time::ptime> times_dt;

  #pragma db value_type("SMALLDATETIME")
  std::vector<boost::posix_time::ptime> times_sdt;

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  std::vector<boost::posix_time::time_duration> durations;
#endif

  bool
  operator== (const object& x) const
  {
    return
      id == x.id
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && dates == x.dates
      && times == x.times
#endif
      && times_dt == x.times_dt
      && times_sdt == x.times_sdt
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && durations == x.durations
#endif
      ;
  }
};

#endif // TEST_HXX
