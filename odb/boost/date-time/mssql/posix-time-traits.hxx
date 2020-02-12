// file      : odb/boost/date-time/mssql/posix-time-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_MSSQL_POSIX_TIME_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_MSSQL_POSIX_TIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <odb/core.hxx>
#include <odb/mssql/traits.hxx>
#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    class default_value_traits< ::boost::posix_time::ptime, id_datetime>
    {
    public:
      typedef ::boost::posix_time::ptime ptime;
      typedef ::boost::posix_time::time_duration time_duration;
      typedef ::boost::gregorian::date date;
      typedef ptime value_type;
      typedef ptime query_type;
      typedef datetime image_type;

      static void
      set_value (ptime& v, const datetime& i, bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
        {
          unsigned long long fract_s (i.fraction);
          fract_s = fract_s * time_duration::ticks_per_second () /
            1000000000ULL;

          v = ptime (
            date (static_cast<date::year_type> (i.year),
                  static_cast<date::month_type> (i.month),
                  static_cast<date::day_type> (i.day)),
            time_duration (
              static_cast<time_duration::hour_type> (i.hour),
              static_cast<time_duration::min_type> (i.minute),
              static_cast<time_duration::sec_type> (i.second),
              static_cast<time_duration::fractional_seconds_type> (fract_s)));
        }
      }

      static void
      set_image (datetime& i, unsigned short s, bool& is_null, const ptime& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else
        {
          const date& d (v.date ());
          const time_duration& t (v.time_of_day ());

          is_null = false;
          i.year = static_cast<SQLSMALLINT> (d.year ());
          i.month = static_cast<SQLUSMALLINT> (d.month ());
          i.day = static_cast<SQLUSMALLINT> (d.day ());
          i.hour = static_cast<SQLUSMALLINT> (t.hours ());
          i.minute = static_cast<SQLUSMALLINT> (t.minutes ());

          // Scale value 8 indicates we are dealing with SMALLDATETIME
          // which has the minutes precision.
          //
          if (s != 8)
          {
            i.second = static_cast<SQLUSMALLINT> (t.seconds ());

            unsigned long long ns (t.fractional_seconds ());
            ns = ns * 1000000000ULL / time_duration::ticks_per_second ();

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
    class default_value_traits< ::boost::posix_time::time_duration, id_time>
    {
    public:
      typedef ::boost::posix_time::time_duration time_duration;
      typedef time_duration value_type;
      typedef time_duration query_type;
      typedef time image_type;

      static void
      set_value (time_duration& v, const time& i, bool is_null)
      {
        if (is_null)
          v = time_duration (::boost::date_time::not_a_date_time);
        else
        {
          unsigned long long fract_s (i.fraction);
          fract_s = fract_s * time_duration::ticks_per_second () /
            1000000000ULL;

          v = time_duration (
            static_cast<time_duration::hour_type> (i.hour),
            static_cast<time_duration::min_type> (i.minute),
            static_cast<time_duration::sec_type> (i.second),
            static_cast<time_duration::fractional_seconds_type> (fract_s));
        }
      }

      static void
      set_image (time& i,
                 unsigned short s,
                 bool& is_null,
                 const time_duration& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else if (v.is_negative () || v.hours () > 23)
          throw odb::boost::date_time::value_out_of_range ();
        else
        {
          is_null = false;
          i.hour = static_cast<SQLUSMALLINT> (v.hours ());
          i.minute = static_cast<SQLUSMALLINT> (v.minutes ());
          i.second = static_cast<SQLUSMALLINT> (v.seconds ());

          unsigned long long ns (v.fractional_seconds ());
          ns = ns * 1000000000ULL / time_duration::ticks_per_second ();

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

          i.fraction = static_cast<SQLUINTEGER> (ns - ns % divider[s]);
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::posix_time::ptime>
    {
      static const database_type_id db_type_id = id_datetime;
    };

    template <>
    struct default_type_traits< ::boost::posix_time::time_duration>
    {
      static const database_type_id db_type_id = id_time;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_MSSQL_POSIX_TIME_TRAITS_HXX
