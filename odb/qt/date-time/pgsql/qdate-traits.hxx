// file      : odb/qt/date-time/pgsql/qdate-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_PGSQL_QDATE_TRAITS_HXX
#define ODB_QT_DATE_TIME_PGSQL_QDATE_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDate>

#include <odb/pgsql/traits.hxx>
#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace pgsql
  {
    // Implementation of the mapping between QDate and PostgreSQL DATE. The
    // DATE values are stored as days since the PostgreSQL epoch 2000-01-01.
    //
    template <>
    struct default_value_traits<QDate, id_date>
    {
      typedef details::endian_traits endian_traits;

      typedef QDate value_type;
      typedef QDate query_type;
      typedef int image_type;

      static void
      set_value (QDate& v, int i, bool is_null)
      {
        if (is_null)
          // A null QDate value is equivalent to an invalid QDate value.
          // Set v to an invalid date to represent null.
          //
          v.setDate (0, 0, 0);
        else
          v = QDate (2000, 1, 1).addDays (endian_traits::ntoh (i));
      }

      static void
      set_image (int& i, bool& is_null, const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i = endian_traits::hton (QDate (2000, 1, 1).daysTo (v));
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

#endif // ODB_QT_DATE_TIME_PGSQL_QDATE_TRAITS_HXX
