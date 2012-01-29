// file      : qt/oracle/date-time/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
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
      time == x.time;
  }

  bool
  is_null () const
  {
    return
      date.isNull () &&
      date_time.isNull () &&
      time.isNull ();
  }

  #pragma db id auto
  unsigned long id;

  QDate date;
  QDateTime date_time;
  QTime time;
};

#endif // TEST_HXX
