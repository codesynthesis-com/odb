// file      : odb/qt/mysql/qdate-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_MYSQL_QDATE_TRAITS_HXX
#define ODB_QT_MYSQL_QDATE_TRAITS_HXX

#include <odb/pre.hxx>

#include <QDate>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    class default_value_traits<QDate, MYSQL_TIME, id_date>
    {
    public:
      typedef QDate value_type;
      typedef QDate query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QDate& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // Set the date to be invalid.
          //
          v.setDate (0, 0, 0);
        else
          v.setDate (i.year, i.month, i.day);
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i.year = v.year ();
          i.month = v.month ();
          i.day = v.day ();
        }
      }
    };
  }
}

#endif // ODB_QT_MYSQL_QDATE_TRAITS_HXX
