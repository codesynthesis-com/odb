// file      : odb/qt/date-time/mssql/qdate-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MSSQL_QDATE_TRAITS_HXX
#define ODB_QT_DATE_TIME_MSSQL_QDATE_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDate>

#include <odb/mssql/traits.hxx>

#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    struct default_value_traits<QDate, id_date>
    {
      typedef QDate value_type;
      typedef QDate query_type;
      typedef date image_type;

      static void
      set_value (QDate& v, const date& i, bool is_null)
      {
        if (is_null)
          // A null QDate value is equivalent to an invalid QDate value.
          // Set v to an invalid date to represent null.
          //
          v.setDate (0, 0, 0);
        else
          v.setDate (i.year, i.month, i.day);
      }

      static void
      set_image (date& i, bool& is_null, const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i.year = static_cast<SQLSMALLINT> (v.year ());
          i.month = static_cast<SQLUSMALLINT> (v.month ());
          i.day = static_cast<SQLUSMALLINT> (v.day ());
        }
      }
    };

    template <>
    struct default_type_traits<QDate>
    {
      static const database_type_id db_type_id = id_date;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_MSSQL_QDATE_TRAITS_HXX
