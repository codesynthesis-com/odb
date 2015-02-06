// file      : odb/qt/date-time/oracle/qtime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_ORACLE_QTIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_ORACLE_QTIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QTime>

#include <odb/oracle/traits.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits<QTime, id_interval_ds>
    {
      typedef QTime value_type;
      typedef QTime query_type;
      typedef interval_ds image_type;

      static void
      set_value (QTime& v, const interval_ds& i, bool is_null)
      {
        if (is_null)
          // A null QTime value is equivalent to an invalid QTime value.
          // Set v to an invalid time to represent null (hour value of
          // a valid time must be in the range 0-23).
          //
          v.setHMS (24, 0, 0);
        else
        {
          sb4 d (0), h (0), m (0), s (0), ns (0);
          i.get (d, h, m, s, ns);

          v.setHMS (h, m, s, ns / 1000000);
        }
      }

      static void
      set_image (interval_ds& i, bool& is_null, const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          i.set (0, v.hour (), v.minute (), v.second (), v.msec () * 1000000);
        }
      }
    };

    template <>
    struct default_type_traits<QTime>
    {
      static const database_type_id db_type_id = id_interval_ds;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_ORACLE_QTIME_TRAITS_HXX
