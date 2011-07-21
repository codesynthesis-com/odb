// file      : odb/qt/date-time/pgsql/qdatetime-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_PGSQL_QDATETIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_PGSQL_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDateTime>

#include <odb/details/buffer.hxx>
#include <odb/pgsql/traits.hxx>
#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace pgsql
  {
    // Implementation of mapping between QDateTime and PostgreSQL TIMESTAMP.
    // TIMESTAMP values are stored as micro-seconds since the PostgreSQL
    // epoch 2000-01-01.
    //
    template <>
    struct default_value_traits<QDateTime, id_timestamp>
    {
      typedef details::endian_traits endian_traits;

      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef long long image_type;

      // The difference between the Unix epoch and the PostgreSQL epoch
      // in seconds.
      //
      static const long long epoch_diff = 946684800LL;

      static void
      set_value (QDateTime& v, long long i, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
          v.setTime_t (
            static_cast <uint> (
              endian_traits::ntoh (i) / 1000000 + epoch_diff));
      }

      static void
      set_image (long long& i, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        // QDateTime::toTime_t returns an unsigned integer. Values less
        // than the Unix epoch are not supported.
        //
        else if (v < QDateTime (QDate (1970, 1, 1),
                                QTime (0, 0, 0),
                                Qt::UTC))
          throw odb::qt::date_time::value_out_of_range ();
        else
        {
          is_null = false;

          long long pg_seconds (static_cast<long long> (v.toTime_t ()) -
                                epoch_diff);

          i = endian_traits::hton (pg_seconds * 1000000);
        }
      }
    };

    template <>
    struct default_type_traits<QDateTime>
    {
      static const database_type_id db_type_id = id_timestamp;
    };
  }
}

#endif // ODB_QT_DATE_TIME_PGSQL_QDATETIME_TRAITS_HXX
