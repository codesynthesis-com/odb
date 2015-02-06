// file      : odb/mssql/traits.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_TRAITS_HXX
#define ODB_MSSQL_TRAITS_HXX

#include <odb/pre.hxx>

#include <odb/details/config.hxx> // ODB_CXX11

#include <string>
#include <vector>
#include <cstddef> // std::size_t
#include <cstring> // std::memcpy, std::memset, std::strlen
#include <cwchar>  // std::wcslen

#ifdef ODB_CXX11
#  include <array>
#endif

#ifdef _WIN32
typedef struct _GUID GUID;
#endif

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

      id_decimal,          // DECIMAL; NUMERIC

      id_smallmoney,
      id_money,

      id_float4,           // REAL; FLOAT(n) with n <= 24
      id_float8,           // FLOAT(n) with n > 24

      id_string,           // CHAR(n), VARCHAR(n) with n <= N
      id_long_string,      // CHAR(n), VARCHAR(n) with n > N; TEXT

      id_nstring,          // NCHAR(n), NVARCHAR(n) with 2*n <= N
      id_long_nstring,     // NCHAR(n), NVARCHAR(n) with 2*n > N; NTEXT

      id_binary,           // BINARY(n), VARBINARY(n) with n <= N
      id_long_binary,      // BINARY(n), VARBINARY(n) with n > N; IMAGE

      id_date,             // DATE
      id_time,             // TIME
      id_datetime,         // DATETIME; DATETIME2; SMALLDATETIME
      id_datetimeoffset,   // DATETIMEOFFSET

      id_uniqueidentifier, // UNIQUEIDENTIFIER
      id_rowversion        // ROWVERSION; TIMESTAMP
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
    struct image_traits<id_datetimeoffset>
    {
      typedef datetimeoffset image_type;
    };

    template <>
    struct image_traits<id_uniqueidentifier>
    {
      typedef uniqueidentifier image_type;
    };

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

      // string, binary.
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

      // nstring.
      //
      static void
      set_value (W& v, const ucs2_char* i, std::size_t n, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      static void
      set_image (ucs2_char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
      }

      // long_string, long_nstring, long_binary.
      //
      static void
      set_value (W& v, result_callback_type& cb, void*& context)
      {
        vtraits::set_value (wtraits::set_ref (v), cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const W& v)
      {
        vtraits::set_image (cb, context, is_null, wtraits::get_ref (v));
      }

      // time, datetime, datetimeoffset.
      //
      static void
      set_image (image_type& i, unsigned short s, bool& is_null, const W& v)
      {
        vtraits::set_image (i, s, is_null, wtraits::get_ref (v));
      }
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

      // string, binary.
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

      // nstring.
      //
      static void
      set_value (W& v, const ucs2_char* i, std::size_t n, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      static void
      set_image (ucs2_char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
      }

      // long_string, long_nstring, long_binary.
      //
      static void
      set_value (W& v, result_callback_type& cb, void*& context)
      {
        // We have to use our own callback since the NULL information
        // is only available during streaming.
        //
        cb = &result_callback;
        context = &v;
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

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);

      // time, datetime, datetimeoffset.
      //
      static void
      set_image (image_type& i, unsigned short s, bool& is_null, const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, s, is_null, wtraits::get_ref (v));
      }
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

    // smallmoney as float/double.
    //
    template <typename T>
    struct smallmoney_float_value_traits
    {
      typedef T value_type;
      typedef T query_type;
      typedef smallmoney image_type;

      static void
      set_value (T& v, const smallmoney& i, bool is_null)
      {
        if (!is_null)
          v = T (i.value / 10000) + T (i.value % 10000) / 10000;
        else
          v = T ();
      }

      static void
      set_image (smallmoney& i, bool& is_null, T v)
      {
        is_null = false;
        i.value = static_cast<int> (v) * 10000 +
          static_cast<int> (v * 10000) % 10000;
      }
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<float, id_smallmoney>:
      smallmoney_float_value_traits<float>
    {
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<double, id_smallmoney>:
      smallmoney_float_value_traits<double>
    {
    };

    // smallmoney as integer.
    //
    template <typename T>
    struct default_value_traits<T, id_smallmoney>
    {
      typedef T value_type;
      typedef T query_type;
      typedef smallmoney image_type;

      static void
      set_value (T& v, const smallmoney& i, bool is_null)
      {
        if (!is_null)
          v = static_cast<T> (i.value);
        else
          v = T ();
      }

      static void
      set_image (smallmoney& i, bool& is_null, T v)
      {
        is_null = false;
        i.value = static_cast<int> (v);
      }
    };

    // money as float/double.
    //
    template <typename T>
    struct money_float_value_traits
    {
      typedef T value_type;
      typedef T query_type;
      typedef money image_type;

      static void
      set_value (T& v, const money& i, bool is_null)
      {
        if (!is_null)
        {
          long long iv ((static_cast<long long> (i.high) << 32) | i.low);
          v = T (iv / 10000) + T (iv % 10000) / 10000;
        }
        else
          v = T ();
      }

      static void
      set_image (money& i, bool& is_null, T v)
      {
        is_null = false;
        long long iv (static_cast<long long> (v) * 10000 +
                      static_cast<long long> (v * 10000) % 10000);
        i.high = static_cast<int> (iv >> 32);
        i.low = static_cast<unsigned int> (iv);
      }
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<float, id_money>:
      money_float_value_traits<float>
    {
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<double, id_money>:
      money_float_value_traits<double>
    {
    };

    // money as integer.
    //
    template <typename T>
    struct default_value_traits<T, id_money>
    {
      typedef T value_type;
      typedef T query_type;
      typedef money image_type;

      static void
      set_value (T& v, const money& i, bool is_null)
      {
        if (!is_null)
        {
          long long iv ((static_cast<long long> (i.high) << 32) | i.low);
          v = static_cast<T> (iv);
        }
        else
          v = T ();
      }

      static void
      set_image (money& i, bool& is_null, T v)
      {
        is_null = false;
        long long iv (static_cast<long long> (v));
        i.high = static_cast<int> (iv >> 32);
        i.low = static_cast<unsigned int> (iv);
      }
    };

    // std::string specialization for string.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::string, id_string>
    {
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
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const std::string& v)
      {
        is_null = false;
        n = v.size ();

        if (n > c)
          n = c;

        if (n != 0)
          std::memcpy (b, v.c_str (), n);
      }
    };

    // char*/const char* specialization for string.
    //
    // Specialization for const char* which only supports initialization
    // of an image from the value but not the other way around. This way
    // we can pass such values to the queries.
    //
    class LIBODB_MSSQL_EXPORT c_string_value_traits
    {
      public:
      typedef const char* value_type;
      typedef char* image_type;

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const char* v)
      {
        is_null = false;
        n = std::strlen (v);

        if (n > c)
          n = c;

        if (n != 0)
          std::memcpy (b, v, n);
      }
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<char*, id_string>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<const char*, id_string>:
      c_string_value_traits {};

    // char[N] specialization.
    //
    struct LIBODB_MSSQL_EXPORT c_array_value_traits_base
    {
      static void
      set_value (char* const& v,
                 const char* b,
                 std::size_t n,
                 bool is_null,
                 std::size_t N);

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const char* v,
                 std::size_t N);
    };

    template <std::size_t N>
    struct default_value_traits<char[N], id_string>
    {
      typedef char* value_type;
      typedef char query_type[N];
      typedef details::buffer image_type;

      static void
      set_value (char* const& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        c_array_value_traits_base::set_value (v, b, n, is_null, N);
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const char* v)
      {
        c_array_value_traits_base::set_image (b, c, n, is_null, v, N);
      }
    };

    // std::array<char, N> (string) specialization.
    //
#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_string>
    {
      typedef std::array<char, N> value_type;
      typedef std::array<char, N> query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        c_array_value_traits_base::set_value (v.data (), b, n, is_null, N);
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        c_array_value_traits_base::set_image (b, c, n, is_null, v.data (), N);
      }
    };
#endif

    // char specialization.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<char, id_string>
    {
      typedef char value_type;
      typedef char query_type;
      typedef details::buffer image_type;

      static void
      set_value (char& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        c_array_value_traits_base::set_value (&v, b, n, is_null, 1);
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 char v)
      {
        c_array_value_traits_base::set_image (b, c, n, is_null, &v, 1);
      }
    };

    // std::string specialization for long_string.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::string,
                                                    id_long_string>
    {
      typedef std::string value_type;
      typedef std::string query_type;
      typedef long_callback image_type;

      static void
      set_value (std::string& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = &v;
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

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

    // char*/const char* specialization for long_string.
    //
    class LIBODB_MSSQL_EXPORT c_string_long_value_traits
    {
    public:
      typedef const char* value_type;
      typedef long_callback image_type;

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

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<
      char*, id_long_string>: c_string_long_value_traits {};

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<
      const char*, id_long_string>: c_string_long_value_traits {};

    // char[N] specialization for long_string.
    //
    template <std::size_t N>
    struct c_array_long_value_traits_base
    {
      static void
      set_value (char* const& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = v;
      }

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

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

    template <std::size_t N>
    struct default_value_traits<char[N], id_long_string>
    {
      typedef char* value_type;
      typedef char query_type[N];
      typedef long_callback image_type;

      static void
      set_value (char* const& v,
                 result_callback_type& cb,
                 void*& context)
      {
        c_array_long_value_traits_base<N>::set_value (v, cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const char* v)
      {
        c_array_long_value_traits_base<N>::set_image (cb, context, is_null, v);
      }
    };

    // std::array<char, N> (long_string) specialization.
    //
#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_long_string>
    {
      typedef std::array<char, N> value_type;
      typedef std::array<char, N> query_type;
      typedef long_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context)
      {
        c_array_long_value_traits_base<N>::set_value (v.data (), cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const value_type& v)
      {
        c_array_long_value_traits_base<N>::set_image (
          cb, context, is_null, v.data ());
      }
    };
#endif

    // std::wstring specialization for nstring.
    //
    template <std::size_t = sizeof (wchar_t)>
    struct wstring_functions
    {
      static void
      assign (std::wstring& s, const ucs2_char* b, std::size_t n)
      {
        s.assign (b, b + n);
      }

      static void
      assign (ucs2_char* b, const wchar_t* s, std::size_t n)
      {
        for (std::size_t i (0); i < n; ++i)
          b[i] = static_cast<ucs2_char> (s[i]);
      }

      // Even though this is not used when ucs2_char == wchar_t, the
      // compiler will still compile the signatures and complain.
      //
#ifndef _WIN32
      static void
      assign (wchar_t* s, const ucs2_char* b, std::size_t n)
      {
        for (std::size_t i (0); i < n; ++i)
          s[i] = static_cast<wchar_t> (b[i]);
      }
#endif
    };

    template <>
    struct LIBODB_MSSQL_EXPORT wstring_functions<sizeof (ucs2_char)>
    {
      static void
      assign (std::wstring& s, const ucs2_char* b, std::size_t n)
      {
        s.assign (reinterpret_cast<const wchar_t*> (b), n);
      }

      static void
      assign (ucs2_char* b, const wchar_t* s, std::size_t n)
      {
        std::memcpy (b, s, n * sizeof (ucs2_char));
      }

      // On Windows ucs2_char is wchar_t which makes this function
      // the same as the one above.
      //
#ifndef _WIN32
      static void
      assign (wchar_t* s, const ucs2_char* b, std::size_t n)
      {
        std::memcpy (s, b, n * sizeof (ucs2_char));
      }
#endif
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::wstring, id_nstring>
    {
      typedef std::wstring value_type;
      typedef std::wstring query_type;
      typedef ucs2_char* image_type;

      typedef wstring_functions<> functions;

      static void
      set_value (std::wstring& v,
                 const ucs2_char* b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          functions::assign (v, b, n);
        else
          v.erase ();
      }

      static void
      set_image (ucs2_char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const std::wstring& v)
      {
        is_null = false;
        n = v.size ();

        if (n > c)
          n = c;

        if (n != 0)
          functions::assign (b, v.c_str (), n);
      }
    };

    // wchar_t*/const wchar_t* specialization for nstring.
    //
    class LIBODB_MSSQL_EXPORT c_wstring_value_traits
    {
    public:
      typedef const wchar_t* value_type;
      typedef ucs2_char* image_type;

      typedef wstring_functions<> functions;

      static void
      set_image (ucs2_char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const wchar_t* v)
      {
        is_null = false;
        n = std::wcslen (v);

        if (n > c)
          n = c;

        if (n != 0)
          functions::assign (b, v, n);
      }
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<wchar_t*, id_nstring>:
      c_wstring_value_traits {};

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<
      const wchar_t*, id_nstring>: c_wstring_value_traits {};

    // wchar_t[N] specialization.
    //
    struct LIBODB_MSSQL_EXPORT c_warray_value_traits_base
    {
      typedef wstring_functions<> functions;

      static void
      set_value (wchar_t* const& v,
                 const ucs2_char* b,
                 std::size_t n,
                 bool is_null,
                 std::size_t N);

      static void
      set_image (ucs2_char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const wchar_t* v,
                 std::size_t N);
    };

    template <std::size_t N>
    struct default_value_traits<wchar_t[N], id_nstring>
    {
      typedef wchar_t* value_type;
      typedef wchar_t query_type[N];
      typedef details::buffer image_type;

      static void
      set_value (wchar_t* const& v,
                 const ucs2_char* b,
                 std::size_t n,
                 bool is_null)
      {
        c_warray_value_traits_base::set_value (v, b, n, is_null, N);
      }

      static void
      set_image (ucs2_char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const wchar_t* v)
      {
        c_warray_value_traits_base::set_image (b, c, n, is_null, v, N);
      }
    };

    // std::array<wchar_t, N> (string) specialization.
    //
#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_value_traits<std::array<wchar_t, N>, id_nstring>
    {
      typedef std::array<wchar_t, N> value_type;
      typedef std::array<wchar_t, N> query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const ucs2_char* b,
                 std::size_t n,
                 bool is_null)
      {
        c_warray_value_traits_base::set_value (v.data (), b, n, is_null, N);
      }

      static void
      set_image (ucs2_char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        c_warray_value_traits_base::set_image (b, c, n, is_null, v.data (), N);
      }
    };
#endif

    // wchar_t specialization.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<wchar_t, id_nstring>
    {
      typedef wchar_t value_type;
      typedef wchar_t query_type;
      typedef details::buffer image_type;

      static void
      set_value (wchar_t& v,
                 const ucs2_char* b,
                 std::size_t n,
                 bool is_null)
      {
        c_warray_value_traits_base::set_value (&v, b, n, is_null, 1);
      }

      static void
      set_image (ucs2_char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 wchar_t v)
      {
        c_warray_value_traits_base::set_image (b, c, n, is_null, &v, 1);
      }
    };

    // std::wstring specialization for long_nstring.
    //
    template <std::size_t = sizeof (wchar_t)>
    struct wstring_long_value_traits;

    template <>
    struct LIBODB_MSSQL_EXPORT wstring_long_value_traits<2>
    {
      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

#ifndef _WIN32
    template <>
    struct LIBODB_MSSQL_EXPORT wstring_long_value_traits<4>
    {
      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };
#endif

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::wstring,
                                                    id_long_nstring>
    {
      typedef std::wstring value_type;
      typedef std::wstring query_type;
      typedef long_callback image_type;

      static void
      set_value (std::wstring& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &wstring_long_value_traits<>::result_callback;
        context = &v;
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const std::wstring& v)
      {
        is_null = false;
        cb = &wstring_long_value_traits<>::param_callback;
        context = &v;
      }
    };

    // wchar_t*/const wchar_t* specialization for long_nstring.
    //
    template <std::size_t = sizeof (wchar_t)>
    struct c_wstring_long_value_traits;

    template <>
    struct LIBODB_MSSQL_EXPORT c_wstring_long_value_traits<2>
    {
      typedef const wchar_t* value_type;
      typedef long_callback image_type;

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const wchar_t* v)
      {
        is_null = false;
        cb = &param_callback;
        context = v;
      }

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);
    };

#ifndef _WIN32
    template <>
    struct LIBODB_MSSQL_EXPORT c_wstring_long_value_traits<4>
    {
      typedef const wchar_t* value_type;
      typedef long_callback image_type;

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const wchar_t* v)
      {
        is_null = false;
        cb = &param_callback;
        context = v;
      }

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);
    };
#endif

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<
      wchar_t*, id_long_nstring>: c_wstring_long_value_traits<> {};

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<
      const wchar_t*, id_long_nstring>: c_wstring_long_value_traits<> {};

    // char[N] specialization for long_nstring.
    //
    template <std::size_t N, std::size_t = sizeof (wchar_t)>
    struct c_warray_long_value_traits_base;

    template <std::size_t N>
    struct c_warray_long_value_traits_base<N, 2>
    {
      static void
      set_value (wchar_t* const& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = v;
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const wchar_t* v)
      {
        is_null = false;
        cb = &param_callback;
        context = v;
      }

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

#ifndef _WIN32
    template <std::size_t N>
    struct c_warray_long_value_traits_base<N, 4>
    {
      static void
      set_value (wchar_t* const& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = v;
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const wchar_t* v)
      {
        is_null = false;
        cb = &param_callback;
        context = v;
      }

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };
#endif

    template <std::size_t N>
    struct default_value_traits<wchar_t[N], id_long_nstring>
    {
      typedef wchar_t* value_type;
      typedef wchar_t query_type[N];
      typedef long_callback image_type;

      static void
      set_value (wchar_t* const& v,
                 result_callback_type& cb,
                 void*& context)
      {
        c_warray_long_value_traits_base<N>::set_value (v, cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const wchar_t* v)
      {
        c_warray_long_value_traits_base<N>::set_image (
          cb, context, is_null, v);
      }
    };

    // std::array<wchar_t, N> (long_nstring) specialization.
    //
#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_value_traits<std::array<wchar_t, N>, id_long_nstring>
    {
      typedef std::array<wchar_t, N> value_type;
      typedef std::array<wchar_t, N> query_type;
      typedef long_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context)
      {
        c_warray_long_value_traits_base<N>::set_value (
          v.data (), cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const value_type& v)
      {
        c_warray_long_value_traits_base<N>::set_image (
          cb, context, is_null, v.data ());
      }
    };
#endif

    // std::vector (buffer) specialization for binary.
    //
    template <typename C>
    struct vector_binary_value_traits
    {
      typedef std::vector<C> value_type;
      typedef std::vector<C> query_type;
      typedef char* image_type;

      static void
      set_value (value_type& v, const char* b, std::size_t n, bool is_null)
      {
        if (!is_null)
        {
          const C* cb (reinterpret_cast<const C*> (b));
          v.assign (cb, cb + n);
        }
        else
          v.clear ();
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        n = v.size ();

        if (n > c)
          n = c;

        // std::vector::data() may not be available in older compilers.
        //
        if (n != 0)
          std::memcpy (b, &v.front (), n);
      }
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::vector<char>,
                                                    id_binary>:
      vector_binary_value_traits<char>
    {
    };

    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::vector<unsigned char>,
                                                    id_binary>:
      vector_binary_value_traits<unsigned char>
    {
    };

    // C array (buffer) specialization for binary.
    //
    template <typename C, std::size_t N>
    struct c_array_binary_value_traits
    {
      typedef C* value_type;
      typedef C query_type[N];
      typedef char* image_type;

      static void
      set_value (C* const& v, const char* b, std::size_t n, bool is_null)
      {
        if (!is_null)
          std::memcpy (v, b, n < N ? n : N);
        else
          std::memset (v, 0, N);
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const C* v)
      {
        is_null = false;
        n = c > N ? N : c;
        std::memcpy (b, v, n);
      }
    };

    template <std::size_t N>
    struct default_value_traits<char[N], id_binary>:
      c_array_binary_value_traits<char, N>
    {
    };

    template <std::size_t N>
    struct default_value_traits<unsigned char[N], id_binary>:
      c_array_binary_value_traits<unsigned char, N>
    {
    };

#ifdef ODB_CXX11
    // std::array (buffer) specialization for binary.
    //
    template <typename C, std::size_t N>
    struct array_binary_value_traits
    {
      typedef std::array<C, N> value_type;
      typedef value_type query_type;
      typedef char* image_type;

      static void
      set_value (value_type& v, const char* b, std::size_t n, bool is_null)
      {
        if (!is_null)
          std::memcpy (v.data (), b, n < N ? n : N);
        else
          std::memset (v.data (), 0, N);
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        n = c > N ? N : c;
        std::memcpy (b, v.data (), n);
      }
    };

    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_binary>:
      array_binary_value_traits<char, N>
    {
    };

    template <std::size_t N>
    struct default_value_traits<std::array<unsigned char, N>, id_binary>:
      array_binary_value_traits<unsigned char, N>
    {
    };
#endif

    // std::vector<char> (buffer) specialization for long_binary.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::vector<char>,
                                                    id_long_binary>
    {
      typedef std::vector<char> value_type;
      typedef std::vector<char> query_type;
      typedef long_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = &v;
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

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

    // std::vector<unsigned char> (buffer) specialization for long_binary.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<std::vector<unsigned char>,
                                                    id_long_binary>
    {
      typedef std::vector<unsigned char> value_type;
      typedef std::vector<unsigned char> query_type;
      typedef long_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = &v;
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

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

    // C array (buffer) specialization for long_binary.
    //
    template <typename C, std::size_t N>
    struct c_array_long_binary_value_traits
    {
      static void
      set_value (C* const& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = v;
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const C* v)
      {
        is_null = false;
        cb = &param_callback;
        context = v;
      }

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

    template <std::size_t N>
    struct default_value_traits<char[N], id_long_binary>:
      c_array_long_binary_value_traits<char, N>
    {
      typedef char* value_type;
      typedef char query_type[N];
      typedef long_callback image_type;
    };

    template <std::size_t N>
    struct default_value_traits<unsigned char[N], id_long_binary>:
      c_array_long_binary_value_traits<unsigned char, N>
    {
      typedef unsigned char* value_type;
      typedef unsigned char query_type[N];
      typedef long_callback image_type;
    };

#ifdef ODB_CXX11
    // std::array (buffer) specialization for long_binary.
    //
    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_long_binary>
    {
      typedef std::array<char, N> value_type;
      typedef value_type query_type;
      typedef long_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context)
      {
        c_array_long_binary_value_traits<char, N>::set_value (
          v.data (), cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const value_type& v)
      {
        c_array_long_binary_value_traits<char, N>::set_image (
          cb, context, is_null, v.data ());
      }
    };

    template <std::size_t N>
    struct default_value_traits<std::array<unsigned char, N>, id_long_binary>
    {
      typedef std::array<unsigned char, N> value_type;
      typedef value_type query_type;
      typedef long_callback image_type;

      static void
      set_value (value_type& v,
                 result_callback_type& cb,
                 void*& context)
      {
        c_array_long_binary_value_traits<unsigned char, N>::set_value (
          v.data (), cb, context);
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const value_type& v)
      {
        c_array_long_binary_value_traits<unsigned char, N>::set_image (
          cb, context, is_null, v.data ());
      }
    };
#endif

    // GUID specialization for uniqueidentifier.
    //
#ifdef _WIN32
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<GUID, id_uniqueidentifier>
    {
      typedef GUID value_type;
      typedef GUID query_type;
      typedef uniqueidentifier image_type;

      static void
      set_value (GUID& v, const uniqueidentifier& i, bool is_null)
      {
        if (!is_null)
          std::memcpy (&v, &i, 16);
        else
          std::memset (&v, 0, 16);
      }

      static void
      set_image (uniqueidentifier& i, bool& is_null, const GUID& v)
      {
        is_null = false;
        std::memcpy (&i, &v, 16);
      }
    };
#endif

    // char[16] specialization for uniqueidentifier.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<char[16],
                                                    id_uniqueidentifier>
    {
      typedef char* value_type;
      typedef const char* query_type;
      typedef uniqueidentifier image_type;

      static void
      set_value (char* const& v, const uniqueidentifier& i, bool is_null)
      {
        if (!is_null)
          std::memcpy (v, &i, 16);
        else
          std::memset (v, 0, 16);
      }

      static void
      set_image (uniqueidentifier& i, bool& is_null, const char* v)
      {
        is_null = false;
        std::memcpy (&i, v, 16);
      }
    };

    // unsigned long long specialization for rowversion.
    //
    template <>
    struct LIBODB_MSSQL_EXPORT default_value_traits<unsigned long long,
                                                    id_rowversion>
    {
      typedef unsigned long long value_type;
      typedef unsigned long long query_type;
      typedef unsigned char* image_type;

      static void
      set_value (unsigned long long& v, const unsigned char* i, bool is_null)
      {
        if (!is_null)
        {
          // The value is in the big-endian format.
          //
          unsigned char* p (reinterpret_cast<unsigned char*> (&v));
          p[0] = i[7];
          p[1] = i[6];
          p[2] = i[5];
          p[3] = i[4];
          p[4] = i[3];
          p[5] = i[2];
          p[6] = i[1];
          p[7] = i[0];
        }
        else
          v = 0;
      }

      static void
      set_image (unsigned char* i, bool& is_null, unsigned long long v)
      {
        is_null = false;

        // The value is in the big-endian format.
        //
        const unsigned char* p (reinterpret_cast<const unsigned char*> (&v));
        i[0] = p[7];
        i[1] = p[6];
        i[2] = p[5];
        i[3] = p[4];
        i[4] = p[3];
        i[5] = p[2];
        i[6] = p[1];
        i[7] = p[0];
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

    // String types.
    //
    template <>
    struct default_type_traits<std::string>
    {
      static const database_type_id db_type_id = id_long_string;
    };

    template <>
    struct default_type_traits<char*>
    {
      static const database_type_id db_type_id = id_long_string;
    };

    template <>
    struct default_type_traits<const char*>
    {
      static const database_type_id db_type_id = id_long_string;
    };

    template <std::size_t N>
    struct default_type_traits<char[N]>
    {
      // Use short string by default to minimize code bloat. Can
      // always be overridden with _val()/_ref().
      //
      static const database_type_id db_type_id = id_string;
    };

#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_type_traits<std::array<char, N> >
    {
      // Ditto.
      //
      static const database_type_id db_type_id = id_string;
    };
#endif

    template <>
    struct default_type_traits<char>
    {
      static const database_type_id db_type_id = id_string;
    };

    // Wide string types.
    //
    template <>
    struct default_type_traits<std::wstring>
    {
      static const database_type_id db_type_id = id_long_nstring;
    };

    template <>
    struct default_type_traits<wchar_t*>
    {
      static const database_type_id db_type_id = id_long_nstring;
    };

    template <>
    struct default_type_traits<const wchar_t*>
    {
      static const database_type_id db_type_id = id_long_nstring;
    };

    template <std::size_t N>
    struct default_type_traits<wchar_t[N]>
    {
      // Use short string by default to minimize code bloat. Can
      // always be overridden with _val()/_ref().
      //
      static const database_type_id db_type_id = id_nstring;
    };

#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_type_traits<std::array<wchar_t, N> >
    {
      // Ditto.
      //
      static const database_type_id db_type_id = id_nstring;
    };
#endif

    template <>
    struct default_type_traits<wchar_t>
    {
      static const database_type_id db_type_id = id_nstring;
    };

    // Binary types.
    //
    template <std::size_t N>
    struct default_type_traits<unsigned char[N]>
    {
      static const database_type_id db_type_id = id_long_binary;
    };

    template <>
    struct default_type_traits<std::vector<char> >
    {
      static const database_type_id db_type_id = id_long_binary;
    };

    template <>
    struct default_type_traits<std::vector<unsigned char> >
    {
      static const database_type_id db_type_id = id_long_binary;
    };

#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_type_traits<std::array<unsigned char, N> >
    {
      static const database_type_id db_type_id = id_long_binary;
    };
#endif

    // GUID.
    //
#ifdef _WIN32
    template <>
    struct default_type_traits<GUID>
    {
      static const database_type_id db_type_id = id_uniqueidentifier;
    };
#endif
  }
}

#include <odb/mssql/traits.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_TRAITS_HXX
