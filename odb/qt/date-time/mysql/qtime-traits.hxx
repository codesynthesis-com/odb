// file      : odb/qt/date-time/mysql/qtime-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
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
          v.setHMS (static_cast<int> (i.hour),
                    static_cast<int> (i.minute),
                    static_cast<int> (i.second));
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
          i.day = 0;

          i.hour = static_cast<unsigned int> (v.hour ());
          i.minute = static_cast<unsigned int> (v.minute ());
          i.second = static_cast<unsigned int> (v.second ());
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

#endif // ODB_QT_DATE_TIME_MYSQL_QTIME_TRAITS_HXX
