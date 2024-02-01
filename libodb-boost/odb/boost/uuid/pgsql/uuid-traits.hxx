// file      : odb/boost/uuid/pgsql/uuid-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_UUID_PGSQL_UUID_TRAITS_HXX
#define ODB_BOOST_UUID_PGSQL_UUID_TRAITS_HXX

#include <boost/version.hpp>

// UUID library is available since 1.42.0.
//
#if BOOST_VERSION >= 104200

#include <odb/pre.hxx>

#include <cstring> // std::memcpy, std::memset

#include <boost/uuid/uuid.hpp>

#include <odb/pgsql/traits.hxx>

namespace odb
{
  namespace pgsql
  {
    template <>
    class default_value_traits< ::boost::uuids::uuid, id_uuid>
    {
    public:
      typedef ::boost::uuids::uuid value_type;
      typedef value_type query_type;
      typedef unsigned char* image_type;

      // PostgreSQL binary UUID representation is big-endian in the RFC 4122,
      // section 4.1.2 order. Lucky for us, that also the representation used
      // by Boost.
      //

      static void
      set_value (value_type& v, const unsigned char* i, bool is_null)
      {
        if (!is_null)
          std::memcpy (v.data, i, 16);
        else
          std::memset (v.data, 0, 16);
      }

      static void
      set_image (unsigned char* i, bool& is_null, const value_type& v)
      {
        // If we can, store nil as NULL. Otherwise, store it as a value.
        //
        is_null = is_null && v.is_nil ();

        if (!is_null)
          std::memcpy (i, v.data, 16);
      }
    };

    template <>
    struct default_type_traits< ::boost::uuids::uuid>
    {
      static const database_type_id db_type_id = id_uuid;
    };
  }
}

#include <odb/post.hxx>

#endif // BOOST_VERSION
#endif // ODB_BOOST_UUID_PGSQL_UUID_TRAITS_HXX
