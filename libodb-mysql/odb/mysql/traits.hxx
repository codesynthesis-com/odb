// file      : odb/mysql/traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_TRAITS_HXX
#define ODB_MYSQL_TRAITS_HXX

#include <odb/pre.hxx>

#include <odb/details/config.hxx> // ODB_CXX11

#include <string>
#include <vector>
#include <cstddef> // std::size_t
#include <cstring> // std::memcpy, std::memset, std::strlen

#ifdef ODB_CXX11
#  include <array>
#endif

#include <odb/traits.hxx>
#include <odb/wrapper-traits.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/mysql-types.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/wrapper-p.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    enum database_type_id
    {
      id_tiny,
      id_utiny,
      id_short,
      id_ushort,
      id_long,
      id_ulong,
      id_longlong,
      id_ulonglong,

      id_float,
      id_double,
      id_decimal,

      id_date,
      id_time,
      id_datetime,
      id_timestamp,
      id_year,

      id_string,
      id_blob,

      id_bit,
      id_enum,
      id_set
    };

    //
    // image_traits
    //

    template <database_type_id>
    struct image_traits;

    template <>
    struct image_traits<id_tiny> {typedef signed char image_type;};

    template <>
    struct image_traits<id_utiny> {typedef unsigned char image_type;};

    template <>
    struct image_traits<id_short> {typedef short image_type;};

    template <>
    struct image_traits<id_ushort> {typedef unsigned short image_type;};

    template <>
    struct image_traits<id_long> {typedef int image_type;};

    template <>
    struct image_traits<id_ulong> {typedef unsigned int image_type;};

    template <>
    struct image_traits<id_longlong> {typedef long long image_type;};

    template <>
    struct image_traits<id_ulonglong> {typedef unsigned long long image_type;};

    template <>
    struct image_traits<id_float> {typedef float image_type;};

    template <>
    struct image_traits<id_double> {typedef double image_type;};

    template <>
    struct image_traits<id_decimal> {typedef details::buffer image_type;};

    template <>
    struct image_traits<id_date> {typedef MYSQL_TIME image_type;};

    template <>
    struct image_traits<id_time> {typedef MYSQL_TIME image_type;};

    template <>
    struct image_traits<id_datetime> {typedef MYSQL_TIME image_type;};

    template <>
    struct image_traits<id_timestamp> {typedef MYSQL_TIME image_type;};

    template <>
    struct image_traits<id_year> {typedef short image_type;};

    template <>
    struct image_traits<id_string> {typedef details::buffer image_type;};

    template <>
    struct image_traits<id_blob> {typedef details::buffer image_type;};

    template <>
    struct image_traits<id_bit> {typedef unsigned char* image_type;};

    // Note: default mapping is to integer. Alternative mapping is to
    // string.
    //
    template <>
    struct image_traits<id_enum> {typedef unsigned short image_type;};

    template <>
    struct image_traits<id_set> {typedef details::buffer image_type;};

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

      // String, BLOB, ENUM, and SET.
      //
      static void
      set_value (W& v, const details::buffer& b, std::size_t n, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), b, n, is_null);
      }

      static void
      set_image (details::buffer& b, std::size_t& n, bool& is_null, const W& v)
      {
        vtraits::set_image (b, n, is_null, wtraits::get_ref (v));
      }

      // BIT.
      //
      static void
      set_value (W& v, const unsigned char* i, std::size_t n, bool is_null)
      {
        vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      static void
      set_image (unsigned char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
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

      // String, BLOB, ENUM, and SET.
      //
      static void
      set_value (W& v, const details::buffer& b, std::size_t n, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), b, n, is_null);
      }

      static void
      set_image (details::buffer& b, std::size_t& n, bool& is_null, const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (b, n, is_null, wtraits::get_ref (v));
      }

      // BIT.
      //
      static void
      set_value (W& v, const unsigned char* i, std::size_t n, bool is_null)
      {
        if (is_null)
          wtraits::set_null (v);
        else
          vtraits::set_value (wtraits::set_ref (v), i, n, is_null);
      }

      static void
      set_image (unsigned char* i,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const W& v)
      {
        is_null = wtraits::get_null (v);

        if (!is_null)
          vtraits::set_image (i, c, n, is_null, wtraits::get_ref (v));
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

    // Specialization for numeric enum representations (C++ enum, integer
    // types, etc). In particular, this specialization works only for C++
    // enum type as long as its numeric value space starts with 0, is
    // ascending and contiguous (i.e., the default enumerator assignment).
    //
    template <typename T>
    struct default_value_traits<T, id_enum>
    {
      typedef T value_type;
      typedef T query_type;
      typedef unsigned short image_type;

      static void
      set_value (T& v, unsigned short i, bool is_null)
      {
        // In MySQL first enumerator has index 1.
        //
        if (!is_null)
          v = static_cast<T> (i - 1);
        else
          v = T ();
      }

      static void
      set_image (unsigned short& i, bool& is_null, const T& v)
      {
        is_null = false;
        i = static_cast<unsigned short> (v) + 1;
      }
    };

    // std::string specialization.
    //
    class LIBODB_MYSQL_EXPORT string_value_traits
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
    struct LIBODB_MYSQL_EXPORT default_value_traits<std::string, id_string>:
      string_value_traits
    {
    };

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<std::string, id_decimal>:
      string_value_traits
    {
    };

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<std::string, id_enum>:
      string_value_traits
    {
    };

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<std::string, id_set>:
      string_value_traits
    {
    };

    // char*/const char* specialization
    //
    // Specialization for const char* which only supports initialization
    // of an image from the value but not the other way around. This way
    // we can pass such values to the queries.
    //
    class LIBODB_MYSQL_EXPORT c_string_value_traits
    {
    public:
      typedef const char* value_type;
      typedef details::buffer image_type;

      static void
      set_image (details::buffer&,
                 std::size_t& n,
                 bool& is_null,
                 const char*);
    };

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<char*, id_string>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<char*, id_decimal>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<char*, id_enum>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<char*, id_set>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<const char*, id_string>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<const char*, id_decimal>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<const char*, id_enum>:
      c_string_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<const char*, id_set>:
      c_string_value_traits {};

    // char[N] specializations.
    //
    struct LIBODB_MYSQL_EXPORT c_array_value_traits_base
    {
      static void
      set_value (char* const& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null,
                 std::size_t N);

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const char* v,
                 std::size_t N);
    };

    template <std::size_t N>
    struct c_array_value_traits
    {
      typedef char* value_type;
      typedef char query_type[N];
      typedef details::buffer image_type;

      static void
      set_value (char* const& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        c_array_value_traits_base::set_value (v, b, n, is_null, N);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const char* v)
      {
        c_array_value_traits_base::set_image (b, n, is_null, v, N);
      }
    };

    template <std::size_t N>
    struct default_value_traits<char[N], id_string>: c_array_value_traits<N>
    {};

    template <std::size_t N>
    struct default_value_traits<char[N], id_decimal>: c_array_value_traits<N>
    {};

    template <std::size_t N>
    struct default_value_traits<char[N], id_enum>: c_array_value_traits<N>
    {};

    template <std::size_t N>
    struct default_value_traits<char[N], id_set>: c_array_value_traits<N>
    {};

    // std::array<char, N> (string) specialization.
    //
#ifdef ODB_CXX11
    template <std::size_t N>
    struct std_array_value_traits
    {
      typedef std::array<char, N> value_type;
      typedef std::array<char, N> query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        c_array_value_traits_base::set_value (v.data (), b, n, is_null, N);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        c_array_value_traits_base::set_image (b, n, is_null, v.data (), N);
      }
    };

    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_string>:
      std_array_value_traits<N> {};

    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_decimal>:
      std_array_value_traits<N> {};

    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_enum>:
      std_array_value_traits<N> {};

    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_set>:
      std_array_value_traits<N> {};
#endif

    // char specialization.
    //
    struct LIBODB_MYSQL_EXPORT char_value_traits
    {
      typedef char value_type;
      typedef char query_type;
      typedef details::buffer image_type;

      static void
      set_value (char& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        c_array_value_traits_base::set_value (&v, b, n, is_null, 1);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 char v)
      {
        c_array_value_traits_base::set_image (b, n, is_null, &v, 1);
      }
    };

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<char, id_string>:
      char_value_traits {};

    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<char, id_enum>:
      char_value_traits {};

    // std::vector<char> (buffer) specialization.
    //
    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<std::vector<char>, id_blob>
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

    // std::vector<unsigned char> (buffer) specialization.
    //
    template <>
    struct LIBODB_MYSQL_EXPORT default_value_traits<
      std::vector<unsigned char>, id_blob>
    {
    public:
      typedef std::vector<unsigned char> value_type;
      typedef std::vector<unsigned char> query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
        {
          const unsigned char* d (
            reinterpret_cast<const unsigned char*> (b.data ()));
          v.assign (d, d + n);
        }
        else
          v.clear ();
      }

      static void
      set_image (details::buffer&,
                 std::size_t& n,
                 bool& is_null,
                 const value_type&);
    };

    // char[N] (buffer) specialization.
    //
    template <std::size_t N>
    struct default_value_traits<char[N], id_blob>
    {
    public:
      typedef char* value_type;
      typedef char query_type[N];
      typedef details::buffer image_type;

      static void
      set_value (char* const& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          std::memcpy (v, b.data (), (n < N ? n : N));
        else
          std::memset (v, 0, N);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const char* v)
      {
        is_null = false;
        n = N;

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), v, n);
      }
    };

    // unsigned char[N] (buffer) specialization.
    //
    template <std::size_t N>
    struct default_value_traits<unsigned char[N], id_blob>
    {
    public:
      typedef unsigned char* value_type;
      typedef unsigned char query_type[N];
      typedef details::buffer image_type;

      static void
      set_value (unsigned char* const& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          std::memcpy (v, b.data (), (n < N ? n : N));
        else
          std::memset (v, 0, N);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const unsigned char* v)
      {
        is_null = false;
        n = N;

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), v, n);
      }
    };

#ifdef ODB_CXX11
    // std::array<char, N> (buffer) specialization.
    //
    template <std::size_t N>
    struct default_value_traits<std::array<char, N>, id_blob>
    {
    public:
      typedef std::array<char, N> value_type;
      typedef value_type query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          std::memcpy (v.data (), b.data (), (n < N ? n : N));
        else
          std::memset (v.data (), 0, N);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        n = N;

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), v.data (), n);
      }
    };

    // std::array<unsigned char, N> (buffer) specialization.
    //
    template <std::size_t N>
    struct default_value_traits<std::array<unsigned char, N>, id_blob>
    {
    public:
      typedef std::array<unsigned char, N> value_type;
      typedef value_type query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          std::memcpy (v.data (), b.data (), (n < N ? n : N));
        else
          std::memset (v.data (), 0, N);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        n = N;

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), v.data (), n);
      }
    };
#endif

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
      static const database_type_id db_type_id = id_tiny;
    };

    template <>
    struct default_type_traits<signed char>
    {
      static const database_type_id db_type_id = id_tiny;
    };

    template <>
    struct default_type_traits<unsigned char>
    {
      static const database_type_id db_type_id = id_utiny;
    };

    template <>
    struct default_type_traits<short>
    {
      static const database_type_id db_type_id = id_short;
    };

    template <>
    struct default_type_traits<unsigned short>
    {
      static const database_type_id db_type_id = id_ushort;
    };

    template <>
    struct default_type_traits<int>
    {
      static const database_type_id db_type_id = id_long;
    };

    template <>
    struct default_type_traits<unsigned int>
    {
      static const database_type_id db_type_id = id_ulong;
    };

    template <>
    struct default_type_traits<long>
    {
      static const database_type_id db_type_id = id_longlong;
    };

    template <>
    struct default_type_traits<unsigned long>
    {
      static const database_type_id db_type_id = id_ulonglong;
    };

    template <>
    struct default_type_traits<long long>
    {
      static const database_type_id db_type_id = id_longlong;
    };

    template <>
    struct default_type_traits<unsigned long long>
    {
      static const database_type_id db_type_id = id_ulonglong;
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

    // String types.
    //
    template <>
    struct default_type_traits<std::string>
    {
      static const database_type_id db_type_id = id_string;
    };

    template <>
    struct default_type_traits<char*>
    {
      static const database_type_id db_type_id = id_string;
    };

    template <>
    struct default_type_traits<const char*>
    {
      static const database_type_id db_type_id = id_string;
    };

    template <std::size_t N>
    struct default_type_traits<char[N]>
    {
      static const database_type_id db_type_id = id_string;
    };

#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_type_traits<std::array<char, N> >
    {
      static const database_type_id db_type_id = id_string;
    };
#endif

    template <>
    struct default_type_traits<char>
    {
      static const database_type_id db_type_id = id_string;
    };

    // Binary types.
    //
    template <std::size_t N>
    struct default_type_traits<unsigned char[N]>
    {
      static const database_type_id db_type_id = id_blob;
    };

    template <>
    struct default_type_traits<std::vector<char> >
    {
      static const database_type_id db_type_id = id_blob;
    };

    template <>
    struct default_type_traits<std::vector<unsigned char> >
    {
      static const database_type_id db_type_id = id_blob;
    };

#ifdef ODB_CXX11
    template <std::size_t N>
    struct default_type_traits<std::array<unsigned char, N> >
    {
      static const database_type_id db_type_id = id_blob;
    };
#endif
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_TRAITS_HXX
