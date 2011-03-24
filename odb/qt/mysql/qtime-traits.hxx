// file      : odb/qt/mysql/qtime-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_MYSQL_QTIME_TRAITS_HXX
#define ODB_QT_MYSQL_QTIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QTime>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    class default_value_traits<QTime, MYSQL_TIME, id_time>
    {
    public:
      typedef QTime value_type;
      typedef QTime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (QTime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          // Set the time to be invalid.
          //
          v.setHMS (24, 0, 0);
        else
          v.setHMS (i.hour, i.minute, i.second);
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i.hour = v.hour ();
          i.minute = v.minute ();
          i.second = v.second ();
        }
      }
    };
  }
}

#endif // ODB_QT_MYSQL_QTIME_TRAITS_HXX
