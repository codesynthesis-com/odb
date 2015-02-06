// file      : odb/qt/date-time/mysql/qdate-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MYSQL_QDATE_TRAITS_HXX
#define ODB_QT_DATE_TIME_MYSQL_QDATE_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDate>

#include <odb/mysql/traits.hxx>
#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    struct default_value_traits<QDate, id_date>
    {
      typedef QDate value_type;
      typedef QDate query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QDate& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // A null QDate value is equivalent to an invalid QDate value.
          // Set v to an invalid date to represent null.
          //
          v.setDate (0, 0, 0);
        else
          v.setDate (static_cast<int> (i.year),
                     static_cast<int> (i.month),
                     static_cast<int> (i.day));
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else if ((v < QDate (1000, 1, 1)) || (v > QDate (9999, 12, 31)))
          throw odb::qt::date_time::value_out_of_range ();
        else
        {
          is_null = false;
          i.neg = false;

          i.year = static_cast<unsigned int> (v.year ());
          i.month = static_cast<unsigned int> (v.month ());
          i.day = static_cast<unsigned int> (v.day ());

          i.hour = 0;
          i.minute = 0;
          i.second = 0;
          i.second_part = 0;
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

#endif // ODB_QT_DATE_TIME_MYSQL_QDATE_TRAITS_HXX
