// file      : odb/boost/date-time/pgsql/posix-time-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_PGSQL_POSIX_TIME_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_PGSQL_POSIX_TIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <odb/core.hxx>
#include <odb/pgsql/traits.hxx>
#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace pgsql
  {
    // Implementation of the mapping between boost::posix_time::ptime and
    // PostgreSQL TIMESTAMP. TIMESTAMP values are stored as micro-seconds
    // since the PostgreSQL epoch 2000-01-01.
    //
    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_timestamp>
    {
      typedef ::boost::gregorian::date date;
      typedef ::boost::posix_time::ptime ptime;
      typedef ::boost::posix_time::time_duration time_duration;

      typedef details::endian_traits endian_traits;

      typedef ptime value_type;
      typedef ptime query_type;
      typedef long long image_type;

      static const long long neg_inf = -0x7fffffffffffffffLL - 1;
      static const long long pos_inf = 0x7fffffffffffffffLL;

      static void
      set_value (ptime& v, long long i, bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
        {
          i = endian_traits::ntoh (i);

          if (i == neg_inf)
            v = ptime (::boost::date_time::neg_infin);
          else if (i == pos_inf)
            v = ptime (::boost::date_time::pos_infin);
          else
          {
            const ptime pg_epoch (date (2000, 1, 1),
                                  time_duration (0, 0, 0));

            // We need to split the microsecond image into hours and
            // microseconds to avoid overflow during the fractional seconds
            // calculation.
            //
            time_duration::hour_type h (
              static_cast<time_duration::hour_type> (i / 3600000000LL));

            i -= static_cast<long long> (h) * 3600000000LL;

            v = pg_epoch +
              time_duration (
                h,
                0,
                0,
                static_cast<time_duration::fractional_seconds_type> (
                  i * time_duration::ticks_per_second () / 1000000LL));
          }
        }
      }

      static void
      set_image (long long& i, bool& is_null, const ptime& v)
      {
        is_null = false;

        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else if (v.is_neg_infinity ())
            i = endian_traits::hton (neg_inf);
          else if (v.is_pos_infinity ())
            i = endian_traits::hton (pos_inf);
          else
            throw odb::boost::date_time::special_value ();
        }
        else
        {
          const ptime pg_epoch (date (2000, 1, 1), time_duration (0, 0, 0));
          i = endian_traits::hton (
            static_cast<long long> ((v - pg_epoch).total_microseconds ()));
        }
      }
    };

    // Implementation of the mapping between boost::posix_time::time_duration
    // and PostgreSQL TIME. The TIME values are stored as micro-seconds since
    // 00:00:00.
    //
    template <>
    struct default_value_traits< ::boost::posix_time::time_duration, id_time>
    {
      typedef ::boost::posix_time::time_duration time_duration;

      typedef details::endian_traits endian_traits;

      typedef time_duration value_type;
      typedef time_duration query_type;
      typedef long long image_type;

      static void
      set_value (time_duration& v, long long i, bool is_null)
      {
        if (is_null)
          v = time_duration (::boost::date_time::not_a_date_time);
        else
          v = time_duration (
            0,
            0,
            0,
            static_cast<time_duration::fractional_seconds_type> (
              endian_traits::ntoh (i) * time_duration::ticks_per_second () /
              1000000LL));
      }

      static void
      set_image (long long& i, bool& is_null, const time_duration& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else if (v.hours () < 0 || v.hours () > 24)
          throw odb::boost::date_time::value_out_of_range ();
        else
        {
          is_null = false;
          i = endian_traits::hton (
            static_cast<long long> (v.total_microseconds ()));
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
      static const database_type_id db_type_id = id_time;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_PGSQL_POSIX_TIME_TRAITS_HXX
