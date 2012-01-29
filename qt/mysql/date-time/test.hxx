// file      : qt/mysql/date-time/test.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
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
      date_time == x.date_time &&
      timestamp == x.timestamp &&
      time == x.time;
  }

  bool
  is_null () const
  {
    return
      date.isNull () &&
      date_time.isNull () &&
      timestamp.isNull () &&
      time.isNull ();
  }

  #pragma db id auto
  unsigned long id;

  QDate date;
  QDateTime date_time;

  // Make timestamp NULL-able to suppress the auto-initialization and
  // auto-update characteristics of the TIMESTAMP datatype, and to
  // allow NULL values.
  //
  #pragma db type("TIMESTAMP") null
  QDateTime timestamp;

  QTime time;
};

#endif // TEST_HXX
