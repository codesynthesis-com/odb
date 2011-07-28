// file      : odb/pgsql/traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_TRAITS_HXX
#define ODB_PGSQL_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/traits.hxx>

#include <odb/details/buffer.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/pgsql-types.hxx>

#include <odb/pgsql/details/export.hxx>
#include <odb/pgsql/details/endian-traits.hxx>

namespace odb
{
  namespace pgsql
  {
    enum database_type_id
    {
      id_boolean,
      id_smallint,
      id_integer,
      id_bigint,

      id_numeric,

      id_real,
      id_double,

      id_date,
      id_time,
      id_timestamp,

      id_string,
      id_bytea,
      id_bit,
      id_varbit,

      id_uuid
    };

    //
    // image_traits
    //

    template <database_type_id>
    struct image_traits;

    template <>
    struct image_traits<id_boolean> {typedef bool image_type;};

    template <>
    struct image_traits<id_smallint> {typedef short image_type;};

    template <>
    struct image_traits<id_integer> {typedef int image_type;};

    template <>
    struct image_traits<id_bigint> {typedef long long image_type;};

    template <>
    struct image_traits<id_numeric> {typedef details::buffer image_type;};

    template <>
    struct image_traits<id_real> {typedef float image_type;};

    template <>
    struct image_traits<id_double> {typedef double image_type;};

    template <>
    struct image_traits<id_date> {typedef int image_type;};

    template <>
    struct image_traits<id_time> {typedef long long image_type;};

    template <>
    struct image_traits<id_timestamp> {typedef long long image_type;};

    template <>
    struct image_traits<id_string> {typedef details::buffer image_type;};

    template <>
    struct image_traits<id_bytea> {typedef details::buffer image_type;};

    template <>
    struct image_traits<id_bit> {typedef unsigned char* image_type;};

    template <>
    struct image_traits<id_varbit> {typedef details::ubuffer image_type;};

    // UUID image is a 16-byte sequence.
    //
    template <>
    struct image_traits<id_uuid> {typedef unsigned char* image_type;};

    //
    // value_traits
    //

    template <typename T, database_type_id>
    struct default_value_traits;

    template <typename T, database_type_id ID>
    class value_traits: public default_value_traits<T, ID>
    {
    };

    template <typename T, database_type_id ID>
    struct default_value_traits
    {
      typedef T value_type;
      typedef T query_type;
      typedef typename image_traits<ID>::image_type image_type;

      static void
      set_value (T& v, const image_type& i, bool is_null)
      {
        if (!is_null)
          v = T (details::endian_traits::ntoh (i));
        else
          v = T ();
      }

      static void
      set_image (image_type& i, bool& is_null, T v)
      {
        is_null = false;
        i = details::endian_traits::hton (image_type (v));
      }
    };

    // std::string specialization.
    //
    class LIBODB_PGSQL_EXPORT string_value_traits
    {
    public:
      typedef std::string value_type;
      typedef std::string query_type;
      typedef details::buffer image_type;

      static void
      set_value (std::string& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          v.assign (b.data (), n);
        else
          v.erase ();
      }

      static void
      set_image (details::buffer&,
                 std::size_t& n,
                 bool& is_null,
                 const std::string&);
    };

    template <>
    struct LIBODB_PGSQL_EXPORT default_value_traits<std::string, id_numeric>:
      string_value_traits
    {
    };

    template <>
    struct LIBODB_PGSQL_EXPORT default_value_traits<std::string, id_string>:
      string_value_traits
    {
    };

    // const char* specialization
    //
    // Specialization for const char* which only supports initialization
    // of an image from the value but not the other way around. This way
    // we can pass such values to the queries.
    //
    class LIBODB_PGSQL_EXPORT c_string_value_traits
    {
    public:
      typedef const char* value_type;
      typedef const char* query_type;
      typedef details::buffer image_type;

      static void
      set_image (details::buffer&,
                 std::size_t& n,
                 bool& is_null,
                 const char*);
    };

    template <>
    struct LIBODB_PGSQL_EXPORT default_value_traits<const char*, id_numeric>:
      c_string_value_traits
    {
    };

    template <>
    struct LIBODB_PGSQL_EXPORT default_value_traits<const char*, id_string>:
      c_string_value_traits
    {
    };

    // std::vector<char> (buffer) specialization.
    //
    template <>
    struct LIBODB_PGSQL_EXPORT default_value_traits<std::vector<char>, id_bytea>
    {
    public:
      typedef std::vector<char> value_type;
      typedef std::vector<char> query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          v.assign (b.data (), b.data () + n);
        else
          v.clear ();
      }

      static void
      set_image (details::buffer&,
                 std::size_t& n,
                 bool& is_null,
                 const value_type&);
    };

    //
    // type_traits
    //

    template <typename T>
    struct default_type_traits;

    template <typename T>
    class type_traits: public default_type_traits<T>
    {
    };

    // Integral types.
    //
    template <>
    struct default_type_traits<bool>
    {
      static const database_type_id db_type_id = id_boolean;
    };

    template <>
    struct default_type_traits<signed char>
    {
      static const database_type_id db_type_id = id_smallint;
    };

    template <>
    struct default_type_traits<unsigned char>
    {
      static const database_type_id db_type_id = id_smallint;
    };

    template <>
    struct default_type_traits<short>
    {
      static const database_type_id db_type_id = id_smallint;
    };

    template <>
    struct default_type_traits<unsigned short>
    {
      static const database_type_id db_type_id = id_smallint;
    };

    template <>
    struct default_type_traits<int>
    {
      static const database_type_id db_type_id = id_integer;
    };

    template <>
    struct default_type_traits<unsigned int>
    {
      static const database_type_id db_type_id = id_integer;
    };

    template <>
    struct default_type_traits<long>
    {
      static const database_type_id db_type_id = id_bigint;
    };

    template <>
    struct default_type_traits<unsigned long>
    {
      static const database_type_id db_type_id = id_bigint;
    };

    template <>
    struct default_type_traits<long long>
    {
      static const database_type_id db_type_id = id_bigint;
    };

    template <>
    struct default_type_traits<unsigned long long>
    {
      static const database_type_id db_type_id = id_bigint;
    };

    // Float types.
    //
    template <>
    struct default_type_traits<float>
    {
      static const database_type_id db_type_id = id_real;
    };

    template <>
    struct default_type_traits<double>
    {
      static const database_type_id db_type_id = id_double;
    };

    // String type.
    //
    template <>
    struct default_type_traits<std::string>
    {
      static const database_type_id db_type_id = id_string;
    };

    template <>
    struct default_type_traits<const char*>
    {
      static const database_type_id db_type_id = id_string;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_TRAITS_HXX
