// file      : odb/qt/date-time/mssql/qdate-time-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MSSQL_QDATETIME_TRAITS_HXX
#define ODB_QT_DATE_TIME_MSSQL_QDATETIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QDateTime>

#include <odb/mssql/traits.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    struct default_value_traits<QDateTime, id_datetime>
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
          v = QDateTime (QDate (i.year,
                                i.month,
                                i.day),
                         QTime (i.hour,
                                i.minute,
                                i.second,
                                static_cast<int> (i.fraction / 1000000)));
      }

      static void
      set_image (datetime& i,
                 unsigned short s,
                 bool& is_null,
                 const QDateTime& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          const QDate& d (v.date ());
          const QTime& t (v.time ());

          is_null = false;
          i.year = static_cast<SQLSMALLINT> (d.year ());
          i.month = static_cast<SQLUSMALLINT> (d.month ());
          i.day = static_cast<SQLUSMALLINT> (d.day ());
          i.hour = static_cast<SQLUSMALLINT> (t.hour ());
          i.minute = static_cast<SQLUSMALLINT> (t.minute ());

          // Scale value 8 indicates we are dealing with SMALLDATETIME
          // which has the minutes precision.
          //
          if (s != 8)
          {
            i.second = static_cast<SQLUSMALLINT> (t.second ());

            const unsigned int divider[8] =
            {
              1000000000,
              100000000,
              10000000,
              1000000,
              100000,
              10000,
              1000,
              100
            };

            unsigned int ns (static_cast<unsigned int> (t.msec ()) * 1000000);
            i.fraction = static_cast<SQLUINTEGER> (ns - ns % divider[s]);
          }
          else
          {
            i.second = 0;
            i.fraction = 0;
          }
        }
      }
    };

    template <>
    struct default_type_traits<QDateTime>
    {
      static const database_type_id db_type_od = id_datetime;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_MSSQL_QDATETIME_TRAITS_HXX
