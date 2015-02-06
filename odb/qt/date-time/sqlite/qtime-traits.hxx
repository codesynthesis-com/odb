// file      : odb/qt/date-time/sqlite/qtime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_SQLITE_QTIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_SQLITE_QTIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t
#include <cstring>  // std::memcpy

#include <QtCore/QTime>

#include <odb/details/buffer.hxx>
#include <odb/sqlite/traits.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    struct default_value_traits<QTime, id_text>
    {
      typedef QTime value_type;
      typedef QTime query_type;
      typedef details::buffer image_type;

      static void
      set_value (QTime& v,
                 const details::buffer& i,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          // A null QTime value is equivalent to an invalid QTime value.
          // Set v to an invalid time to represent null (hour value of
          // a valid time must be in the range 0-23).
          //
          v.setHMS (24, 0, 0);
        else
          v = QTime::fromString (
            QString::fromLatin1 (i.data (), static_cast<int> (n)),
            "HH:mm:ss.zzz");
      }

      static void
      set_image (details::buffer& i,
                 std::size_t& n,
                 bool& is_null,
                 const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          // Cannot use toStdString() here since Qt could have been
          // configured without the STL compatibility support.
          //
          std::string s (
            v.toString ("HH:mm:ss.zzz").toLatin1 ().constData ());

          n = s.size ();
          if (n > i.capacity ())
            i.capacity (n);

          std::memcpy (i.data (), s.data (), n);
        }
      }
    };

    // Implementation of mapping between QTime and SQLite INTEGER. The
    // integer value represents seconds since midnight.
    //
    template <>
    struct default_value_traits<QTime, id_integer>
    {
      typedef QTime value_type;
      typedef QTime query_type;
      typedef long long image_type;

      static void
      set_value (QTime& v, long long i, bool is_null)
      {
        if (is_null)
          // A null QTime value is equivalent to an invalid QTime value.
          // Set v to an invalid time to represent null (hour value of
          // a valid time must be in the range 0-23).
          //
          v.setHMS (24, 0, 0);
        else
          v = QTime (0, 0, 0).addSecs (static_cast<int> (i));
      }

      static void
      set_image (long long& i, bool& is_null, const QTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          i = static_cast<long long> (QTime (0, 0, 0).secsTo (v));
        }
      }
    };

    template <>
    struct default_type_traits<QTime>
    {
      static const database_type_id db_type_id = id_text;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_SQLITE_QTIME_TRAITS_HXX
