// file      : odb/qt/date-time/mysql/qtime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MYSQL_QTIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_MYSQL_QTIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QTime>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    struct default_value_traits<QTime, id_time>
    {
      typedef QTime value_type;
      typedef QTime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QTime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // A null QTime value is equivalent to an invalid QTime value.
          // Set v to an invalid time to represent null (hour value of
          // a valid time must be in the range 0-23).
          //
          v.setHMS (24, 0, 0);
        else
          // Since MySQL 5.6.4, the microseconds part is no longer ignored.
          //
          v.setHMS (static_cast<int> (i.hour),
                    static_cast<int> (i.minute),
                    static_cast<int> (i.second),
                    static_cast<int> (i.second_part / 1000));
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i.neg = false;

          i.year = 0;
          i.month = 0;
          i.day = 0;

          i.hour = static_cast<unsigned int> (v.hour ());
          i.minute = static_cast<unsigned int> (v.minute ());
          i.second = static_cast<unsigned int> (v.second ());
          i.second_part = static_cast<unsigned long> (v.msec ()) * 1000;
        }
      }
    };

    template <>
    struct default_type_traits<QTime>
    {
      static const database_type_id db_type_id = id_time;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_MYSQL_QTIME_TRAITS_HXX
