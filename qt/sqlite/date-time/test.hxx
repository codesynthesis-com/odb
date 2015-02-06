// file      : qt/sqlite/date-time/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <QtCore/QDateTime>

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
      unix_day == x.unix_day &&
      time == x.time &&
      seconds == x.seconds &&
      date_time == x.date_time &&
      unix_time == x.unix_time;
  }

  bool
  is_null () const
  {
    return
      date.isNull () &&
      unix_day.isNull () &&
      time.isNull () &&
      seconds.isNull () &&
      date_time.isNull () &&
      unix_time.isNull ();
  }

  #pragma db id auto
  unsigned long id;

  QDate date;

  #pragma db type ("INTEGER")
  QDate unix_day;

  QTime time;

  #pragma db type ("INTEGER")
  QTime seconds;

  QDateTime date_time;

  #pragma db type ("INTEGER")
  QDateTime unix_time;
};

#endif // TEST_HXX
