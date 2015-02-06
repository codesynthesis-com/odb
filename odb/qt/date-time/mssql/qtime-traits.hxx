// file      : odb/qt/date-time/mssql/qtime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MSSQL_QTIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_MSSQL_QTIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QTime>

#include <odb/mssql/traits.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    struct default_value_traits<QTime, id_time>
    {
      typedef QTime value_type;
      typedef QTime query_type;
      typedef time image_type;

      static void
      set_value (QTime& v, const time& i, bool is_null)
      {
        if (is_null)
          // A null QTime value is equivalent to an invalid QTime value.
          // Set v to an invalid time to represent null (hour value of
          // a valid time must be in the range 0-23).
          //
          v.setHMS (24, 0, 0);
        else
          v.setHMS (i.hour,
                    i.minute,
                    i.second,
                    static_cast<int> (i.fraction / 1000000));
      }

      static void
      set_image (time& i, unsigned short s, bool& is_null, const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i.hour = static_cast<SQLUSMALLINT> (v.hour ());
          i.minute = static_cast<SQLUSMALLINT> (v.minute ());
          i.second = static_cast<SQLUSMALLINT> (v.second ());

          const unsigned int divider[8] =
          {
            1000000000,
            100000000,
            10000000,
            1000000,
            100000,
            10000,
            1000,
            100
          };

          unsigned int ns (static_cast<unsigned int> (v.msec ()) * 1000000);
          i.fraction = static_cast<SQLUINTEGER> (ns - ns % divider[s]);
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

#endif // ODB_QT_DATE_TIME_MSSQL_QTIME_TRAITS_HXX
