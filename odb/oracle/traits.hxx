// file      : odb/oracle/traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_TRAITS_HXX
#define ODB_ORACLE_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/traits.hxx>
#include <odb/wrapper-traits.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-types.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/wrapper-p.hxx>

#include <odb/oracle/details/export.hxx>
#include <odb/oracle/details/number.hxx>

namespace odb
{
  namespace oracle
  {
    enum database_type_id
    {
      id_int32,
      id_int64,

      id_big_int,

      id_float,
      id_double,

      // Both large fixed-point and large floating point NUMBER and FLOAT
      // values are mapped to this id.
      //
      id_big_float,

      id_date,
      id_timestamp,

      id_string,
      id_nstring,

      id_raw,

      id_blob,
      id_clob,
      id_nclob
    };

    //
    // int_traits
    //

    // Only mark fundamental unsigned integers as unsigned. In particular,
    // treat enums as signed since in most cases and on most platforms the
    // underlying integer type will be signed. On Windows with VC9 and up
    // and with GCC, the __intN types are simply aliases for the respective
    // standard integers so the below code will cover them as well. Also
    // note that the ODB compiler performs a similar test, so if you change
    // anything below you will probably also need to make a similar change
    // there.
    //
    template <typename T>
    struct int_traits {static const bool unsign = false;};

    template <>
    struct int_traits<bool> {static const bool unsign = true;};
    template <>
    struct int_traits<unsigned char> {static const bool unsign = true;};
    template <>
    struct int_traits<unsigned short> {static const bool unsign = true;};
    template <>
    struct int_traits<unsigned int> {static const bool unsign = true;};
    template <>
    struct int_traits<unsigned long> {static const bool unsign = true;};
    template <>
    struct int_traits<unsigned long long>
    {
      static const bool unsign = true;
    };

    //
    // image_traits
    //

    template <typename T, database_type_id>
    struct image_traits;

    // int32
    //
    template <bool unsign>
    struct int32_image_traits;

    template <>
    struct int32_image_traits<false>
    {
      typedef int image_type;
    };

    template <>
    struct int32_image_traits<true>
    {
      typedef unsigned int image_type;
    };

    template <typename T>
    struct image_traits<T, id_int32>: int32_image_traits<int_traits<T>::unsign>
    {
    };

    // int64
    //
    template <bool unsign>
    struct int64_image_traits;

    template <>
    struct int64_image_traits<false>
    {
      typedef long long image_type;
    };

    template <>
    struct int64_image_traits<true>
    {
      typedef unsigned long long image_type;
    };

    template <typename T>
    struct image_traits<T, id_int64>: int64_image_traits<int_traits<T>::unsign>
    {
    };

    // big_int
    //
    template <typename T>
    struct image_traits<T, id_big_int>
    {
      // Image is a buffer containing native OCI NUMBER representation.
      //
      typedef char* image_type;
    };

    template <typename T>
    struct image_traits<T, id_float> {typedef float image_type;};

    template <typename T>
    struct image_traits<T, id_double> {typedef double image_type;};

    template <typename T>
    struct image_traits<T, id_big_float>
    {
      // Image is a buffer containing the native OCI NUMBER representation.
      //
      typedef char* image_type;
    };

    template <typename T>
    struct image_traits<T, id_date>
    {
      // Image is a buffer containing the native OCI DATE representation. This
      // buffer has a fixed length of 7 bytes.
      //
      typedef char* image_type;
    };

    template <typename T>
    struct image_traits<T, id_timestamp>
    {
      // Image is a buffer containing the native OCI TIMESTAMP representation.
      // This buffer has varying length, depending on the microsecond
      // precision of the TIMESTAMP value.
      //
      typedef char* image_type;
    };

    template <typename T>
    struct image_traits<T, id_string> {typedef char* image_type;};

    template <typename T>
    struct image_traits<T, id_nstring> {typedef char* image_type;};

    template <typename T>
    struct image_traits<T, id_raw> {typedef char* image_type;};

    template <typename T>
    struct image_traits<T, id_blob> {typedef lob_callback image_type;};

    template <typename T>
    struct image_traits<T, id_clob> {typedef lob_callback image_type;};

    template <typename T>
    struct image_traits<T, id_nclob> {typedef lob_callback image_type;};

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
      typedef typename wtraits::wrapped_type wrapped_type;

      typedef W value_type;
      typedef wrapped_type query_type;
      typedef typename image_traits<wrapped_type, ID>::image_type image_type;

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

      // big_int, big_float.
      //
      static void
      set_value (W& v, const char* i, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), i, is_null);
      }

      static void
      set_image (char* i, std::size_t& n, bool& is_null, const W& v)
      {
        vtraits::set_image (i, n, is_null, wtraits::get_ref (v));
      }

      // timestamp, string, nstring, raw.
      //
      static void
      set_value (W& v, const char* i, std::size_t n, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      static void
      set_image (char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
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
    };

    template <typename W, database_type_id ID>
    struct wrapped_value_traits<W, ID, true>
    {
      typedef wrapper_traits<W> wtraits;
      typedef typename wtraits::wrapped_type wrapped_type;

      typedef W value_type;
      typedef wrapped_type query_type;
      typedef typename image_traits<wrapped_type, ID>::image_type image_type;

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

      // big_int, big_float.
      //
      static void
      set_value (W& v, const char& i, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), i, is_null);
      }

      static void
      set_image (char* i, std::size_t& n, bool& is_null, const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, n, is_null, wtraits::get_ref (v));
      }

      // timestamp, string, nstring, raw.
      //
      static void
      set_value (W& v, const char* i, std::size_t n, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

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
    };

    template <typename T, database_type_id ID>
    struct default_value_traits
    {
      typedef T value_type;
      typedef T query_type;
      typedef typename image_traits<T, ID>::image_type image_type;

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

    // id_big_int partial specialization.
    //
    template <typename T, bool unsign>
    struct big_int_value_traits;

    template <typename T>
    struct big_int_value_traits<T, false>
    {
      static void
      set_value (T v, const char* b, bool is_null)
      {
        if (!is_null)
          v = static_cast<T> (details::number_to_int64 (b));
        else
          v = 0;
      }

      static void
      set_image (char* b, std::size_t& n, bool& is_null, T v)
      {
        is_null = false;
        details::int64_to_number (b, n, static_cast<long long> (v));
      }
    };

    template <typename T>
    struct big_int_value_traits<T, true>
    {
      static void
      set_value (T v, const char* b, bool is_null)
      {
        if (!is_null)
          v = static_cast<T> (details::number_to_uint64 (b));
        else
          v = 0;
      }

      static void
      set_image (char* b, std::size_t& n, bool& is_null, T v)
      {
        is_null = false;
        details::uint64_to_number (b, n, static_cast<unsigned long long> (v));
      }
    };

    template <typename T>
    struct default_value_traits<T, id_big_int>:
      big_int_value_traits<T, int_traits<T>::unsign>
    {
      typedef T value_type;
      typedef T query_type;
      typedef typename image_traits<T, id_big_int>::image_type image_type;
    };

    // std::string specialization.
    //
    class LIBODB_ORACLE_EXPORT string_value_traits
    {
    public:
      typedef std::string value_type;
      typedef std::string query_type;
      typedef char* image_type;

      static void
      set_value (std::string& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          v.assign (b, n);
        else
          v.erase ();
      }

      static void
      set_image (char*,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const std::string&);
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<std::string, id_string>:
      string_value_traits
    {
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<std::string, id_nstring>:
      string_value_traits
    {
    };

    // const char* specialization.
    //
    // Specialization for const char* which only supports initialization
    // of an image from the value but not the other way around. This way
    // we can pass such values to the queries.
    //
    class LIBODB_ORACLE_EXPORT c_string_value_traits
    {
    public:
      typedef const char* value_type;
      typedef const char* query_type;
      typedef char* image_type;

      static void
      set_image (char*,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const char*);
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<const char*, id_string>:
      c_string_value_traits
    {
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<const char*, id_nstring>:
      c_string_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<char[n], id_string>: c_string_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<const char[n], id_string>:
      c_string_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<char[n], id_nstring>: c_string_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<const char[n], id_nstring>:
      c_string_value_traits
    {
    };

    // std::vector specialization for RAW.
    //
    template <>
    struct default_value_traits<std::vector<char>, id_raw>
    {
    public:
      typedef std::vector<char> value_type;
      typedef std::vector<char> query_type;
      typedef lob_callback image_type;

      static void
      set_value (value_type& v, const char* b, std::size_t n, bool is_null)
      {
        if (!is_null)
          v.assign (b, b + n);
        else
          v.clear ();
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v);
    };

    // std::string specialization for LOBs.
    //
    class string_lob_value_traits
    {
    public:
      typedef std::string value_type;
      typedef std::string query_type;
      typedef lob_callback image_type;

      static void
      set_value (std::string& v,
                 result_callback_type& cb,
                 void*& context,
                 bool is_null)
      {
        if (!is_null)
        {
          cb = &result_callback;
          context = &v;
        }
        else
          v.erase ();
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const std::string& v)
      {
        is_null = false;
        cb = &param_callback;
        context = &v;
      }

      static bool
      result_callback (void* context, void* buffer, ub4 size, chunk_position);

      static bool
      param_callback (const void* context,
                      ub4* position_context,
                      const void** buffer,
                      ub4* size,
                      chunk_position*,
                      void* temp_buffer,
                      ub4 capacity);
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<std::string, id_clob>:
      string_lob_value_traits
    {
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<std::string, id_nclob>:
      string_lob_value_traits
    {
    };

    // const char* specialization for LOBs.
    //
    // Specialization for const char* which only supports initialization
    // of an image from the value but not the other way around. This way
    // we can pass such values to the queries.
    //
    class c_string_lob_value_traits
    {
    public:
      typedef const char* value_type;
      typedef const char* query_type;
      typedef lob_callback image_type;

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const char* v)
      {
        is_null = false;
        cb = &param_callback;
        context = v;
      }

      static bool
      param_callback (const void* context,
                      ub4* position_context,
                      const void** buffer,
                      ub4* size,
                      chunk_position*,
                      void* temp_buffer,
                      ub4 capacity);
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<const char*, id_clob>:
      c_string_lob_value_traits
    {
    };

    template <>
    struct LIBODB_ORACLE_EXPORT default_value_traits<const char*, id_nclob>:
      c_string_lob_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<char[n], id_clob>:
      c_string_lob_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<const char[n], id_clob>:
      c_string_lob_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<char[n], id_nclob>:
      c_string_lob_value_traits
    {
    };

    template <std::size_t n>
    struct default_value_traits<const char[n], id_nclob>:
      c_string_lob_value_traits
    {
    };
    // std::vector<char> (buffer) specialization.
    //
    template <>
    struct default_value_traits<std::vector<char>, id_blob>
    {
    public:
      typedef std::vector<char> value_type;
      typedef std::vector<char> query_type;
      typedef lob_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context,
                 bool is_null)
      {
        if (!is_null)
        {
          cb = &result_callback;
          context = &v;
        }
        else
          v.clear ();
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        cb = &param_callback;
        context = &v;
      }

      static bool
      result_callback (void* context, void* buffer, ub4 size, chunk_position);

      static bool
      param_callback (const void* context,
                      ub4* position_context,
                      const void** buffer,
                      ub4* size,
                      chunk_position*,
                      void* temp_buffer,
                      ub4 capacity);
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
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<signed char>
    {
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<unsigned char>
    {
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<short>
    {
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<unsigned short>
    {
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<int>
    {
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<unsigned int>
    {
      static const database_type_id db_type_id = id_int32;
    };

    template <>
    struct default_type_traits<long>
    {
      static const database_type_id db_type_id = id_big_int;
    };

    template <>
    struct default_type_traits<unsigned long>
    {
      static const database_type_id db_type_id = id_big_int;
    };

    template <>
    struct default_type_traits<long long>
    {
      static const database_type_id db_type_id = id_big_int;
    };

    template <>
    struct default_type_traits<unsigned long long>
    {
      static const database_type_id db_type_id = id_big_int;
    };

    // Float types.
    //
    template <>
    struct default_type_traits<float>
    {
      static const database_type_id db_type_id = id_float;
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

    template <std::size_t n>
    struct default_type_traits<char[n]>
    {
      static const database_type_id db_type_id = id_string;
    };

    template <std::size_t n>
    struct default_type_traits<const char[n]>
    {
      static const database_type_id db_type_id = id_string;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_TRAITS_HXX
