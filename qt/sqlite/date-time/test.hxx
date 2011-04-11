// file      : qt/sqlite/date-time/test.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <QDateTime>

#include <odb/core.hxx>

#pragma db object
struct object
{
  bool
  operator== (const object& x) const
  {
    return
      id == x.id &&
      date == x.date &&
      julian_day == x.julian_day &&
      time == x.time &&
      seconds == x.seconds &&
      date_time == x.date_time;
  }

  bool
  is_null () const
  {
    return
      date.isNull () &&
      julian_day.isNull () &&
      time.isNull () &&
      seconds.isNull () &&
      date_time.isNull ();
  }

  #pragma db id auto
  unsigned long id;

  QDate date;

  #pragma db type ("INTEGER")
  QDate julian_day;

  QTime time;

  #pragma db type ("INTEGER")
  QTime seconds;

  QDateTime date_time;
};

#endif // TEST_HXX
