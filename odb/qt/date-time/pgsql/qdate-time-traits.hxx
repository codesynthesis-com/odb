// file      : odb/qt/date-time/pgsql/qdatetime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_PGSQL_QDATETIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_PGSQL_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDateTime>

#include <odb/pgsql/traits.hxx>

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

      static void
      set_value (QDateTime& v, long long i, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
        {
          const QDateTime pg_epoch (QDate (2000, 1, 1), QTime (0, 0, 0));
          v = pg_epoch.addMSecs (
            static_cast <qint64> (endian_traits::ntoh (i) / 1000LL));
        }
      }

      static void
      set_image (long long& i, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          const QDateTime pg_epoch (QDate (2000, 1, 1), QTime (0, 0, 0));
          i = endian_traits::hton (
            static_cast<long long> (pg_epoch.msecsTo (v)) * 1000LL);
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

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_PGSQL_QDATETIME_TRAITS_HXX
