// file      : odb/boost/date-time/oracle/gregorian-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_ORACLE_GREGORIAN_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_ORACLE_GREGORIAN_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/date_time/gregorian/gregorian_types.hpp>

#include <odb/core.hxx>

#include <odb/oracle/traits.hxx>
#include <odb/oracle/details/date.hxx>

#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits< ::boost::gregorian::date, id_date>
    {
      typedef ::boost::gregorian::date date;
      typedef date value_type;
      typedef date query_type;
      typedef char* image_type;

      static void
      set_value (date& v, const char* b, bool is_null)
      {
        if (is_null)
          v = date (::boost::date_time::not_a_date_time);
        else
        {
          short y;
          unsigned char m, d, h, minute, s;

          details::get_date (b, y, m, d, h, minute, s);

          v = date (static_cast<date::year_type> (y),
                    static_cast<date::month_type> (m),
                    static_cast<date::day_type> (d));
        }
      }

      static void
      set_image (char* b, bool& is_null, const date& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else
        {
          is_null = false;

          details::set_date (b,
                             static_cast<short> (v.year ()),
                             static_cast<unsigned char> (v.month ()),
                             static_cast<unsigned char> (v.day ()),
                             0,
                             0,
                             0);
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::gregorian::date>
    {
      static const database_type_id db_type_id = id_date;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_ORACLE_GREGORIAN_TRAITS_HXX
