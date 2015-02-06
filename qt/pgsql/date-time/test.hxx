// file      : qt/pgsql/date-time/test.hxx
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
      time == x.time &&
      date_time == x.date_time;
  }

  bool
  is_null () const
  {
    return
      date.isNull () &&
      time.isNull () &&
      date_time.isNull ();
  }

  #pragma db id auto
  unsigned long id;

  QDate date;
  QTime time;
  QDateTime date_time;
};

#endif // TEST_HXX
