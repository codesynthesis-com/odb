// file      : odb/qt/date-time/oracle/qdate-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_ORACLE_QDATE_TRAITS_HXX
#define ODB_QT_DATE_TIME_ORACLE_QDATE_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDate>

#include <odb/oracle/traits.hxx>

#include <odb/oracle/details/date.hxx>

#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits<QDate, id_date>
    {
      typedef QDate value_type;
      typedef QDate query_type;
      typedef char* image_type;

      static void
      set_value (QDate& v, const char* b, bool is_null)
      {
        if (is_null)
          // A null QDate value is equivalent to an invalid QDate value.
          // Set v to an invalid date to represent null.
          //
          v.setDate (0, 0, 0);
        else
        {
          short y (0);
          unsigned char m (0), d (0), h (0), minute (0), s (0);
          details::get_date (b, y, m, d, h, minute, s);

          v.setDate (y, m, d);
        }
      }

      static void
      set_image (char* b, bool& is_null, const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          details::set_date (b,
                             static_cast<short> (v.year ()),
                             static_cast<unsigned char> (v.month ()),
                             static_cast<unsigned char> (v.day ()),
                             0,
                             0,
                             0);
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

#endif // ODB_QT_DATE_TIME_ORACLE_QDATE_TRAITS_HXX
