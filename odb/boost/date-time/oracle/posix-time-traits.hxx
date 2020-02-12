// file      : odb/boost/date-time/oracle/posix-time-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_ORACLE_POSIX_TIME_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_ORACLE_POSIX_TIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <odb/core.hxx>

#include <odb/oracle/traits.hxx>
#include <odb/oracle/oracle-fwd.hxx>   // ub1, sb2, ub4
#include <odb/oracle/oracle-types.hxx> // odb::oracle::{datetime interval_ds}
#include <odb/oracle/details/date.hxx>

#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_timestamp>
    {
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
          sb2 y;
          ub1 m, d, h, minute, s;
          ub4 ns;

          i.get (y, m, d, h, minute, s, ns);

          unsigned long long fract_s (ns);
          fract_s = fract_s * time_duration::ticks_per_second () /
            1000000000ULL;

          v = ptime (
            date (static_cast<date::year_type> (y),
                  static_cast<date::month_type> (m),
                  static_cast<date::day_type> (d)),
            time_duration (
              static_cast<time_duration::hour_type> (h),
              static_cast<time_duration::min_type> (minute),
              static_cast<time_duration::sec_type> (s),
              static_cast<time_duration::fractional_seconds_type> (fract_s)));
        }
      }

      static void
      set_image (datetime& i, bool& is_null, const ptime& v)
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
          is_null = false;

          const date& d (v.date ());
          const time_duration& t (v.time_of_day ());

          unsigned long long ns (t.fractional_seconds ());
          ns = ns * 1000000000ULL / time_duration::ticks_per_second ();

          i.set (static_cast<sb2> (d.year ()),
                 static_cast<ub1> (d.month ()),
                 static_cast<ub1> (d.day ()),
                 static_cast<ub1> (t.hours ()),
                 static_cast<ub1> (t.minutes ()),
                 static_cast<ub1> (t.seconds ()),
                 static_cast<ub4> (ns));
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_date>
    {
      typedef ::boost::posix_time::ptime ptime;
      typedef ::boost::posix_time::time_duration time_duration;
      typedef ::boost::gregorian::date date;

      typedef ptime value_type;
      typedef ptime query_type;
      typedef char* image_type;

      static void
      set_value (ptime& v, const char* b, bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
        {
          short y;
          unsigned char m, d, h, minute, s;

          details::get_date (b, y, m, d, h, minute, s);

          v = ptime (
            date (static_cast<date::year_type> (y),
                  static_cast<date::month_type> (m),
                  static_cast<date::day_type> (d)),
            time_duration (
              static_cast<time_duration::hour_type> (h),
              static_cast<time_duration::min_type> (minute),
              static_cast<time_duration::sec_type> (s),
              0));
        }
      }

      static void
      set_image (char* b, bool& is_null, const ptime& v)
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
          is_null = false;

          const date& d (v.date ());
          const time_duration& t (v.time_of_day ());

          details::set_date (b,
                             static_cast<short> (d.year ()),
                             static_cast<unsigned char> (d.month ()),
                             static_cast<unsigned char> (d.day ()),
                             static_cast<unsigned char> (t.hours ()),
                             static_cast<unsigned char> (t.minutes ()),
                             static_cast<unsigned char> (t.seconds ()));
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::posix_time::time_duration,
                                 id_interval_ds>
    {
      typedef ::boost::posix_time::time_duration time_duration;

      typedef time_duration value_type;
      typedef time_duration query_type;
      typedef interval_ds image_type;

      static void
      set_value (time_duration& v,
                 const interval_ds& i,
                 bool is_null)
      {
        if (is_null)
          v = time_duration (::boost::date_time::not_a_date_time);
        else
        {
          sb4 d, h, m, s, ns;
          i.get (d, h, m, s, ns);

          unsigned long long fract_s (ns);
          fract_s = fract_s * time_duration::ticks_per_second () /
            1000000000ULL;

          v = time_duration (
            static_cast<time_duration::hour_type> (
              static_cast<unsigned long long> (d) * 24 + h),
            static_cast<time_duration::min_type> (m),
            static_cast<time_duration::sec_type> (s),
            static_cast<time_duration::fractional_seconds_type> (fract_s));
        }
      }

      static void
      set_image (interval_ds& i,
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
        else
        {
          is_null = false;

          unsigned long long ns (v.fractional_seconds ());
          ns = ns * 1000000000ULL / time_duration::ticks_per_second ();

          i.set (0,
                 static_cast<sb4> (v.hours ()),
                 static_cast<sb4> (v.minutes ()),
                 static_cast<sb4> (v.seconds ()),
                 static_cast<sb4> (ns));
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::posix_time::ptime>
    {
      static const database_type_id db_type_id = id_timestamp;
    };

    template <>
    struct default_type_traits< ::boost::posix_time::time_duration>
    {
      static const database_type_id db_type_id = id_interval_ds;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_ORACLE_POSIX_TIME_TRAITS_HXX
