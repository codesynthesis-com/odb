// file      : odb/qt/date-time/mysql/qdate-time-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MYSQL_QDATETIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_MYSQL_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDateTime>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    struct default_value_traits<QDateTime, id_datetime>
    {
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QDateTime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
          // Since MySQL 5.6.4, the microseconds part is no longer ignored.
          //
          v = QDateTime (QDate (static_cast<int> (i.year),
                                static_cast<int> (i.month),
                                static_cast<int> (i.day)),
                         QTime (static_cast<int> (i.hour),
                                static_cast<int> (i.minute),
                                static_cast<int> (i.second),
                                static_cast<int> (i.second_part / 1000)));
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          if ((v < QDateTime (QDate (1000, 1, 1))) ||
              (v >= QDateTime (QDate (10000, 1, 1))))
            throw odb::qt::date_time::value_out_of_range ();

          is_null = false;
          i.neg = false;

          const QDate& d (v.date ());
          i.year = static_cast<unsigned int> (d.year ());
          i.month = static_cast<unsigned int> (d.month ());
          i.day = static_cast<unsigned int> (d.day ());

          const QTime& t (v.time ());
          i.hour = static_cast<unsigned int> (t.hour ());
          i.minute = static_cast<unsigned int> (t.minute ());
          i.second = static_cast<unsigned int> (t.second ());
          i.second_part = static_cast<unsigned long> (t.msec ()) * 1000;
        }
      }
    };

    template <>
    struct default_value_traits<QDateTime, id_timestamp>
    {
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QDateTime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
          // Since MySQL 5.6.4, the microseconds part is no longer ignored.
          //
          v = QDateTime (QDate (static_cast<int> (i.year),
                                static_cast<int> (i.month),
                                static_cast<int> (i.day)),
                         QTime (static_cast<int> (i.hour),
                                static_cast<int> (i.minute),
                                static_cast<int> (i.second),
                                static_cast<int> (i.second_part / 1000)));
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          if ((v <= QDateTime (QDate (1970, 1, 1))) ||
              (v > QDateTime (QDate (2038, 1, 19), QTime (3, 14, 7))))
            throw odb::qt::date_time::value_out_of_range ();

          is_null = false;
          i.neg = false;

          const QDate& d (v.date ());
          i.year = static_cast<unsigned int> (d.year ());
          i.month = static_cast<unsigned int> (d.month ());
          i.day = static_cast<unsigned int> (d.day ());

          const QTime& t (v.time ());
          i.hour = static_cast<unsigned int> (t.hour ());
          i.minute = static_cast<unsigned int> (t.minute ());
          i.second = static_cast<unsigned int> (t.second ());
          i.second_part = static_cast<unsigned long> (t.msec ()) * 1000;
        }
      }
    };

    template <>
    struct default_type_traits<QDateTime>
    {
      static const database_type_id db_type_od = id_datetime;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_MYSQL_QDATETIME_TRAITS_HXX
