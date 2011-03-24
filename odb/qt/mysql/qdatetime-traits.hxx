// file      : odb/qt/mysql/qdatetime-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_MYSQL_QDATETIME_TRAITS_HXX
#define ODB_QT_MYSQL_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QDateTime>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    class default_value_traits<QDateTime, MYSQL_TIME, id_datetime>
    {
    public:
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QDateTime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // Set the date part to be invalid.
          //
          v.setDate (QDate (0, 0, 0));
        else
        {
          v.setDate (QDate (i.year, i.month, i.day));
          v.setTime (QTime (i.hour, i.minute, i.second));
        }
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QDateTime& v)
      {
        if (v.is_null ())
          is_null = true;
        else
        {
          is_null = false;

          const QDate& d (v.date ());
          i.year = d.year;
          i.month = d.month;
          i.day = d.day;

          const QTime& t (v.time ());
          i.hour = t.hour ();
          i.minute = t.minute ();
          i.second = t.second ();
        }
      }
    };

    class default_value_traits<QDateTime, MYSQL_TIME, id_timestamp>
    {
    public:
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QDateTime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // Set the date part to be invalid.
          //
          v.setDate (QDate (0, 0, 0));
        else
        {
          v.setDate (QDate (i.year, i.month, i.day));
          v.setDate (QTime (i.hour, i.minute, i.second));
        }
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QDateTime& v)
      {
        if (v.is_null ())
          is_null = true;
        else
        {
          is_null = false;

          const QDate& d (v.date ());
          i.year = d.year;
          i.month = d.month;
          i.day = d.day;

          const QTime& t (v.time ());
          i.hour = t.hour ();
          i.minute = t.minute ();
          i.second = t.second ();
        }
      }
    };
  }
}

#endif // ODB_QT_MYSQL_QDATETIME_TRAITS_HXX
