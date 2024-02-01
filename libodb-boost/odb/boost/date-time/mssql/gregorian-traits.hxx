// file      : odb/boost/date-time/mssql/gregorian-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_MSSQL_GREGORIAN_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_MSSQL_GREGORIAN_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/date_time/gregorian/gregorian_types.hpp>

#include <odb/core.hxx>
#include <odb/mssql/traits.hxx>
#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    class default_value_traits< ::boost::gregorian::date, id_date>
    {
    public:
      typedef ::boost::gregorian::date value_type;
      typedef value_type query_type;
      typedef date image_type;

      static void
      set_value (value_type& v, const date& i, bool is_null)
      {
        if (is_null)
          v = value_type (::boost::date_time::not_a_date_time);
        else
          v = value_type (static_cast<value_type::year_type> (i.year),
                          static_cast<value_type::month_type> (i.month),
                          static_cast<value_type::day_type> (i.day));
      }

      static void
      set_image (date& i, bool& is_null, const value_type& v)
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
          i.year = static_cast<SQLSMALLINT> (v.year ());
          i.month = static_cast<SQLUSMALLINT> (v.month ());
          i.day = static_cast<SQLUSMALLINT> (v.day ());
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

#endif // ODB_BOOST_DATE_TIME_MSSQL_GREGORIAN_TRAITS_HXX
