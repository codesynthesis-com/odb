// file      : odb/boost/uuid/mssql/uuid-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_UUID_MSSQL_UUID_TRAITS_HXX
#define ODB_BOOST_UUID_MSSQL_UUID_TRAITS_HXX

#include <boost/version.hpp>

// UUID library is available since 1.42.0.
//
#if BOOST_VERSION >= 104200

#include <odb/pre.hxx>

#include <cstring> // std::memcpy, std::memset

#include <boost/uuid/uuid.hpp>

#include <odb/mssql/traits.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    class default_value_traits< ::boost::uuids::uuid, id_uniqueidentifier>
    {
    public:
      typedef ::boost::uuids::uuid value_type;
      typedef value_type query_type;
      typedef uniqueidentifier image_type;

      static void
      set_value (value_type& v, const uniqueidentifier& i, bool is_null)
      {
        if (!is_null)
          std::memcpy (v.data, &i, 16);
        else
          std::memset (v.data, 0, 16);
      }

      static void
      set_image (uniqueidentifier& i, bool& is_null, const value_type& v)
      {
        // If we can, store nil as NULL. Otherwise, store it as a value.
        //
        is_null = is_null && v.is_nil ();

        if (!is_null)
          std::memcpy (&i, v.data, 16);
      }
    };

    template <>
    struct default_type_traits< ::boost::uuids::uuid>
    {
      static const database_type_id db_type_id = id_uniqueidentifier;
    };
  }
}

#include <odb/post.hxx>

#endif // BOOST_VERSION
#endif // ODB_BOOST_UUID_MSSQL_UUID_TRAITS_HXX
