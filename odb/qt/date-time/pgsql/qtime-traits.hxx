// file      : odb/qt/date-time/pgsql/qtime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_PGSQL_QTIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_PGSQL_QTIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QTime>

#include <odb/pgsql/traits.hxx>

namespace odb
{
  namespace pgsql
  {
    // Implementation of the mapping between QTime and PostgreSQL TIME. The
    // TIME values are stored as micro-seconds since 00:00:00.
    //
    template <>
    struct default_value_traits<QTime, id_time>
    {
      typedef details::endian_traits endian_traits;

      typedef QTime value_type;
      typedef QTime query_type;
      typedef long long image_type;

      static void
      set_value (QTime& v, long long i, bool is_null)
      {
        if (is_null)
          // A null QTime value is equivalent to an invalid QTime value.
          // Set v to an invalid time to represent null (hour value of
          // a valid time must be in the range 0-23).
          //
          v.setHMS (24, 0, 0);
        else
        {
          const QTime base (0, 0, 0);

          v = base.addMSecs (
            static_cast<int> (endian_traits::ntoh (i) / 1000));
        }
      }

      static void
      set_image (long long& i, bool& is_null, const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          const QTime base (0, 0, 0);

          i = endian_traits::hton (
            static_cast<long long> (base.msecsTo (v)) * 1000);
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

#endif // ODB_QT_DATE_TIME_PGSQL_QTIME_TRAITS_HXX
