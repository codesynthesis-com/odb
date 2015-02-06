// file      : qt/mssql/date-time/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <QtCore/QDateTime>

#include <odb/core.hxx>

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id;

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  QDate date;

  QDateTime date_time;
#endif

  #pragma db type ("DATETIME")
  QDateTime date_time_dt;

  #pragma db type ("SMALLDATETIME")
  QDateTime date_time_sdt;

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  QTime time;
#endif

  bool
  operator== (const object& x) const
  {
    return
      id == x.id
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && date == x.date
      && date_time == x.date_time
#endif
      && date_time_dt == x.date_time_dt
      && date_time_sdt == x.date_time_sdt
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && time == x.time
#endif
      ;
  }

  bool
  is_null () const
  {
    return
      true
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && date.isNull ()
      && date_time.isNull ()
#endif
      && date_time_dt.isNull ()
      && date_time_sdt.isNull ()
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && time.isNull ()
#endif
      ;
  }
};

#endif // TEST_HXX
