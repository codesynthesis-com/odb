// file      : odb/boost/uuid/mysql/uuid-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_UUID_MYSQL_UUID_TRAITS_HXX
#define ODB_BOOST_UUID_MYSQL_UUID_TRAITS_HXX

#include <boost/version.hpp>

// UUID library is available since 1.42.0.
//
#if BOOST_VERSION >= 104200

#include <odb/pre.hxx>

#include <cstring> // std::memcpy, std::memset
#include <cassert>

#include <boost/uuid/uuid.hpp>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    struct default_value_traits< ::boost::uuids::uuid, id_blob>
    {
      typedef ::boost::uuids::uuid value_type;
      typedef value_type query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
        {
          assert (n == 16);
          std::memcpy (v.data, b.data (), 16);
        }
        else
          std::memset (v.data, 0, 16);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        // If we can, store nil as NULL. Otherwise, store it as a value.
        //
        is_null = is_null && v.is_nil ();

        if (!is_null)
        {
          n = 16;

          if (n > b.capacity ())
            b.capacity (n);

          std::memcpy (b.data (), v.data, n);
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::uuids::uuid>
    {
      static const database_type_id db_type_id = id_blob;
    };
  }
}

#include <odb/post.hxx>

#endif // BOOST_VERSION
#endif // ODB_BOOST_UUID_MYSQL_UUID_TRAITS_HXX
