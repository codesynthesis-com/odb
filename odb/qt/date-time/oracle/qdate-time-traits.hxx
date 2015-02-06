// file      : odb/qt/date-time/oracle/qdate-time-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_ORACLE_QDATETIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_ORACLE_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDateTime>

#include <odb/oracle/traits.hxx>
#include <odb/oracle/details/date.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits<QDateTime, id_timestamp>
    {
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef datetime image_type;

      static void
      set_value (QDateTime& v, const datetime& i, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
        {
          sb2 y (0);
          ub1 m (0), d (0), h (0), minute (0), s (0);
          ub4 ns (0);
          i.get (y, m, d, h, minute, s, ns);

          v = QDateTime (QDate (static_cast<int> (y),
                                static_cast<int> (m),
                                static_cast<int> (d)),
                         QTime (static_cast<int> (h),
                                static_cast<int> (minute),
                                static_cast<int> (s),
                                static_cast<int> (ns / 1000000)));
        }
      }

      static void
      set_image (datetime& i, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          const QDate& d (v.date ());
          const QTime& t (v.time ());

          i.set (static_cast<sb2> (d.year ()),
                 static_cast<ub1> (d.month ()),
                 static_cast<ub1> (d.day ()),
                 static_cast<ub1> (t.hour ()),
                 static_cast<ub1> (t.minute ()),
                 static_cast<ub1> (t.second ()),
                 static_cast<ub4> (t.msec () * 1000000));
        }
      }
    };

    template <>
    struct default_value_traits<QDateTime, id_date>
    {
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef char* image_type;

      static void
      set_value (QDateTime& v, const char* b, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
        {
          short y;
          unsigned char m, d, h, minute, s;

          details::get_date (b, y, m, d, h, minute, s);

          v = QDateTime (QDate (static_cast<int> (y),
                                static_cast<int> (m),
                                static_cast<int> (d)),
                         QTime (static_cast<int> (h),
                                static_cast<int> (minute),
                                static_cast<int> (s),
                                0));
        }
      }

      static void
      set_image (char* b, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          const QDate& d (v.date ());
          const QTime& t (v.time ());

          details::set_date (b,
                             static_cast<short> (d.year ()),
                             static_cast<unsigned char> (d.month ()),
                             static_cast<unsigned char> (d.day ()),
                             static_cast<unsigned char> (t.hour ()),
                             static_cast<unsigned char> (t.minute ()),
                             static_cast<unsigned char> (t.second ()));
        }
      }
    };

    template <>
    struct default_type_traits<QDateTime>
    {
      static const database_type_id db_type_od = id_timestamp;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_ORACLE_QDATETIME_TRAITS_HXX
