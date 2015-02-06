// file      : qt/oracle/date-time/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

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
      date_time == x.date_time &&
      date_time_d == x.date_time_d &&
      time == x.time;
  }

  bool
  is_null () const
  {
    return
      date.isNull () &&
      date_time.isNull () &&
      date_time_d.isNull () &&
      time.isNull ();
  }

  #pragma db id auto
  unsigned long id;

  QDate date;
  QDateTime date_time;
  #pragma db type("DATE")
  QDateTime date_time_d;
  QTime time;
};

#endif // TEST_HXX
