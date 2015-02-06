// file      : odb/qt/date-time/sqlite/qdatetime-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_SQLITE_QDATETIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_SQLITE_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t
#include <cstring>  // std::memcpy

#include <QtCore/QDateTime>

#include <odb/details/buffer.hxx>
#include <odb/sqlite/traits.hxx>
#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    struct default_value_traits<QDateTime, id_text>
    {
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef details::buffer image_type;

      static void
      set_value (QDateTime& v,
                 const details::buffer& i,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
          v = QDateTime::fromString (
            QString::fromLatin1 (i.data (), static_cast<int> (n)),
            "yyyy-MM-ddTHH:mm:ss.zzz");
      }

      static void
      set_image (details::buffer& i,
                 std::size_t& n,
                 bool& is_null,
                 const QDateTime& v)
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
            v.toString ("yyyy-MM-ddTHH:mm:ss.zzz").toLatin1 ().constData ());

          n = s.size ();
          if (n > i.capacity ())
            i.capacity (n);

          std::memcpy (i.data (), s.data (), n);
        }
      }
    };

    // Implementation of mapping between QDateTime and SQLite INTEGER.
    // The integer value represents UNIX time.
    //
    template <>
    struct default_value_traits<QDateTime, id_integer>
    {
      typedef QDateTime value_type;
      typedef QDateTime query_type;
      typedef long long image_type;

      static void
      set_value (QDateTime& v, long long i, bool is_null)
      {
        if (is_null)
          // Default constructor creates a null QDateTime.
          //
          v = QDateTime ();
        else
        {
          v.setTimeSpec (Qt::UTC);
          v.setTime_t (static_cast <uint> (i));
        }
      }

      static void
      set_image (long long& i, bool& is_null, const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else if (v < QDateTime (QDate (1970, 1, 1),
                                QTime (0, 0, 0),
                                Qt::UTC))
          throw odb::qt::date_time::value_out_of_range ();
        else
        {
          is_null = false;
          i = static_cast<long long> (v.toTime_t ());
        }
      }
    };

    template <>
    struct default_type_traits<QDateTime>
    {
      static const database_type_id db_type_id = id_text;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_SQLITE_QDATETIME_TRAITS_HXX
