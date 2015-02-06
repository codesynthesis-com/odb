// file      : odb/qt/date-time/sqlite/qdate-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_SQLITE_QDATE_TRAITS_HXX
#define ODB_QT_DATE_TIME_SQLITE_QDATE_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t
#include <cstring>  // std::memcpy

#include <QtCore/QDate>

#include <odb/details/buffer.hxx>
#include <odb/sqlite/traits.hxx>
#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    struct default_value_traits<QDate, id_text>
    {
      typedef QDate value_type;
      typedef QDate query_type;
      typedef details::buffer image_type;

      static void
      set_value (QDate& v,
                 const details::buffer& i,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          // A null QDate value is equivalent to an invalid QDate value.
          // Set v to an invalid date to represent null.
          //
          v.setDate (0, 0, 0);
        else
          v = QDate::fromString (
            QString::fromLatin1 (i.data (), static_cast<int> (n)),
            "yyyy-MM-dd");
      }

      static void
      set_image (details::buffer& i,
                 std::size_t& n,
                 bool& is_null,
                 const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          // Cannot use toStdString() here since Qt could have been
          // configured without the STL compatibility support.
          //
          std::string s (v.toString ("yyyy-MM-dd").toLatin1 ().constData ());

          n = s.size ();
          if (n > i.capacity ())
            i.capacity (n);

          std::memcpy (i.data (), s.data (), n);
        }
      }
    };

    // Implementation of the mapping between QDate and SQLite INTEGER. The
    // integer value represents UNIX time.
    //
    template <>
    struct default_value_traits<QDate, id_integer>
    {
      typedef QDate value_type;
      typedef QDate query_type;
      typedef long long image_type;

      static void
      set_value (QDate& v, long long i, bool is_null)
      {
        if (is_null)
          // A null QDate value is equivalent to an invalid QDate value.
          // Set v to an invalid date to represent null.
          //
          v.setDate (0, 0, 0);
        else
        {
          QDateTime dt;
          dt.setTimeSpec (Qt::UTC);
          dt.setTime_t (static_cast<uint> (i));

          v = dt.date ();
        }
      }

      static void
      set_image (long long& i, bool& is_null, const QDate& v)
      {
        if (v.isNull ())
          is_null = true;
        else if (v < QDate (1970, 1, 1))
          throw odb::qt::date_time::value_out_of_range ();
        else
        {
          is_null = false;
          i = static_cast<long long> (
            QDateTime (v, QTime (0, 0, 0), Qt::UTC).toTime_t ());
        }
      }
    };

    template <>
    struct default_type_traits<QDate>
    {
      static const database_type_id db_type_id = id_text;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_SQLITE_QDATE_TRAITS_HXX
