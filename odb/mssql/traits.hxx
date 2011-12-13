// file      : odb/mssql/traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_TRAITS_HXX
#define ODB_MSSQL_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t
//@@ #include <cstring> // std::memcpy, std::memset, std::strlen
//@@ #include <cassert>

#include <odb/traits.hxx>
#include <odb/wrapper-traits.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/mssql-types.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/wrapper-p.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    enum database_type_id
    {
      id_bit,
      id_tinyint,
      id_smallint,
      id_int,
      id_bigint,

      id_decimal,        // DECIMAL; NUMERIC

      id_smallmoney,
      id_money,

      id_float4,         // REAL; FLOAT(n) with n <= 24
      id_float8,         // FLOAT(n) with n > 24

      id_string,         // CHAR(n), VARCHAR(n) with n <= N
      id_long_string,    // CHAR(n), VARCHAR(n) with n > N; TEXT

      id_nstring,        // NCHAR(n), NVARCHAR(n) with 2*n <= N
      id_long_nstring,   // NCHAR(n), NVARCHAR(n) with 2*n > N; NTEXT

      id_binary,         // BINARY(n), VARBINARY(n) with n <= N
      id_long_binary,    // BINARY(n), VARBINARY(n) with n > N; IMAGE

      id_date,           // DATE
      id_time,           // TIME
      id_datetime,       // DATETIME; DATETIME2; SMALLDATETIME
      id_datetimeoffset, // DATETIMEOFFSET

      id_uuid,           // UNIQUEIDENTIFIER
      id_rowversion      // ROWVERSION; TIMESTAMP
    };

    //
    // image_traits
    //

    template <database_type_id>
    struct image_traits;

    template <>
    struct image_traits<id_bit> {typedef unsigned char image_type;};

    template <>
    struct image_traits<id_tinyint> {typedef unsigned char image_type;};

    template <>
    struct image_traits<id_smallint> {typedef short image_type;};

    template <>
    struct image_traits<id_int> {typedef int image_type;};

    template <>
    struct image_traits<id_bigint> {typedef long long image_type;};

    template <>
    struct image_traits<id_decimal> {typedef decimal image_type;};

    template <>
    struct image_traits<id_smallmoney> {typedef smallmoney image_type;};

    template <>
    struct image_traits<id_money> {typedef money image_type;};

    template <>
    struct image_traits<id_float4> {typedef float image_type;};

    template <>
    struct image_traits<id_float8> {typedef double image_type;};

    template <>
    struct image_traits<id_string> {typedef char* image_type;};

    template <>
    struct image_traits<id_long_string> {typedef long_callback image_type;};

    template <>
    struct image_traits<id_nstring> {typedef ucs2_char* image_type;};

    template <>
    struct image_traits<id_long_nstring> {typedef long_callback image_type;};

    template <>
    struct image_traits<id_binary> {typedef char* image_type;};

    template <>
    struct image_traits<id_long_binary> {typedef long_callback image_type;};

    template <>
    struct image_traits<id_date> {typedef date image_type;};

    template <>
    struct image_traits<id_time> {typedef time image_type;};

    template <>
    struct image_traits<id_datetime> {typedef datetime image_type;};

    template <>
    struct image_traits<id_datetimeoffset> {typedef datetimeoffset image_type;};

    // Image is a 16-byte sequence.
    //
    template <>
    struct image_traits<id_uuid> {typedef unsigned char* image_type;};

    // Image is an 8-byte sequence.
    //
    template <>
    struct image_traits<id_rowversion> {typedef unsigned char* image_type;};

    //
    // value_traits
    //

    template <typename W, database_type_id, bool null_handler>
    struct wrapped_value_traits;

    template <typename T, database_type_id>
    struct default_value_traits;

    template <typename T, database_type_id, bool w = details::wrapper_p<T>::r>
    struct select_traits;

    template <typename T, database_type_id ID>
    struct select_traits<T, ID, false>
    {
      typedef default_value_traits<T, ID> type;
    };

    template <typename W, database_type_id ID>
    struct select_traits<W, ID, true>
    {
      typedef
      wrapped_value_traits<W, ID, wrapper_traits<W>::null_handler>
      type;
    };

    template <typename T, database_type_id ID>
    class value_traits: public select_traits<T, ID>::type
    {
    };

    // The wrapped_value_traits specializations should be able to handle
    // any value type which means we have to have every possible signature
    // of the set_value() and set_image() functions.
    //
    template <typename W, database_type_id ID>
    struct wrapped_value_traits<W, ID, false>
    {
      typedef wrapper_traits<W> wtraits;
      typedef typename wtraits::unrestricted_wrapped_type wrapped_type;

      typedef W value_type;
      typedef wrapped_type query_type;
      typedef typename image_traits<ID>::image_type image_type;

      typedef value_traits<wrapped_type, ID> vtraits;

      static void
      set_value (W& v, const image_type& i, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), i, is_null);
      }

      static void
      set_image (image_type& i, bool& is_null, const W& v)
      {
        vtraits::set_image (i, is_null, wtraits::get_ref (v));
      }

      /*
        @@ TODO

      // big_int, big_float, string, nstring, raw.
      //
      static void
      set_value (W& v, const char* i, std::size_t n, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      // string, nstring, raw.
      //
      static void
      set_image (char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
      }

      // big_int, big_float.
      //
      static void
      set_image (char* i, std::size_t& n, bool& is_null, const W& v)
      {
        vtraits::set_image (i, n, is_null, wtraits::get_ref (v));
      }

      // blob, clob, nclob.
      //
      static void
      set_value (W& v, result_callback_type& cb, void*& context, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), cb, context, is_null);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const W& v)
      {
        vtraits::set_image (cb, context, is_null, wtraits::get_ref (v));
      }
      */
    };

    template <typename W, database_type_id ID>
    struct wrapped_value_traits<W, ID, true>
    {
      typedef wrapper_traits<W> wtraits;
      typedef typename wtraits::unrestricted_wrapped_type wrapped_type;

      typedef W value_type;
      typedef wrapped_type query_type;
      typedef typename image_traits<ID>::image_type image_type;

      typedef value_traits<wrapped_type, ID> vtraits;

      static void
      set_value (W& v, const image_type& i, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), i, is_null);
      }

      static void
      set_image (image_type& i, bool& is_null, const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, is_null, wtraits::get_ref (v));
      }

      /*
        @@ TODO

      // big_int, big_float, string, nstring, raw.
      //
      static void
      set_value (W& v, const char* i, std::size_t n, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      // string, nstring, raw.
      //
      static void
      set_image (char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
      }

      // big_int, big_float
      //
      static void
      set_image (char* i, std::size_t& n, bool& is_null, const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, n, is_null, wtraits::get_ref (v));
      }

      // blob, clob, nclob.
      //
      static void
      set_value (W& v, result_callback_type& cb, void*& context, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), cb, context, is_null);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (cb, context, is_null, wtraits::get_ref (v));
      }
      */
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
          v = T (i);
        else
          v = T ();
      }

      static void
      set_image (image_type& i, bool& is_null, T v)
      {
        is_null = false;
        i = image_type (v);
      }
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
      static const database_type_id db_type_id = id_bit;
    };

    template <>
    struct default_type_traits<signed char>
    {
      static const database_type_id db_type_id = id_tinyint;
    };

    template <>
    struct default_type_traits<unsigned char>
    {
      static const database_type_id db_type_id = id_tinyint;
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
      static const database_type_id db_type_id = id_int;
    };

    template <>
    struct default_type_traits<unsigned int>
    {
      static const database_type_id db_type_id = id_int;
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
      static const database_type_id db_type_id = id_float4;
    };

    template <>
    struct default_type_traits<double>
    {
      static const database_type_id db_type_id = id_float8;
    };

    // String type.
    //
    template <>
    struct default_type_traits<std::string>
    {
      static const database_type_id db_type_id = id_long_string;
    };

    template <>
    struct default_type_traits<const char*>
    {
      static const database_type_id db_type_id = id_long_string;
    };

    template <std::size_t n>
    struct default_type_traits<char[n]>
    {
      static const database_type_id db_type_id = id_long_string;
    };

    template <std::size_t n>
    struct default_type_traits<const char[n]>
    {
      static const database_type_id db_type_id = id_long_string;
    };
  }
}

#include <odb/mssql/traits.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_TRAITS_HXX
