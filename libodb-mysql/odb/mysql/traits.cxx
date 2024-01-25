// file      : odb/mysql/traits.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/mysql/traits.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
    using details::buffer;

    //
    // string_value_traits
    //

    void string_value_traits::
    set_image (buffer& b,
               size_t& n,
               bool& is_null,
               const string& v)
    {
      is_null = false;
      n = v.size ();

      if (n > b.capacity ())
        b.capacity (n);

      if (n != 0)
        memcpy (b.data (), v.c_str (), n);
    }

    //
    // c_string_value_traits
    //

    void c_string_value_traits::
    set_image (buffer& b,
               size_t& n,
               bool& is_null,
               const char* v)
    {
      is_null = false;
      n = strlen (v);

      if (n > b.capacity ())
        b.capacity (n);

      if (n != 0)
        memcpy (b.data (), v, n);
    }

    //
    // c_array_value_traits_base
    //
    void c_array_value_traits_base::
    set_value (char* const& v,
               const details::buffer& b,
               size_t n,
               bool is_null,
               size_t N)
    {
      if (!is_null)
      {
        n = n < N ? n : N;

        if (n != 0)
          memcpy (v, b.data (), n);
      }
      else
        n = 0;

      if (n != N) // Append '\0' if there is space.
        v[n] = '\0';
    }

    void c_array_value_traits_base::
    set_image (details::buffer& b,
               size_t& n,
               bool& is_null,
               const char* v,
               size_t N)
    {
      is_null = false;

      // Figure out the length. We cannot use strlen since it may
      // not be 0-terminated (strnlen is not standard).
      //
      for (n = 0; n != N && v[n] != '\0'; ++n) ;

      if (n > b.capacity ())
        b.capacity (n);

      if (n != 0)
        memcpy (b.data (), v, n);
    }

    //
    // default_value_traits<vector<char>, id_blob>
    //
    // std::vector has to be qualified for Sun CC.
    //
    void default_value_traits<std::vector<char>, id_blob>::
    set_image (details::buffer& b,
               size_t& n,
               bool& is_null,
               const value_type& v)
    {
      is_null = false;
      n = v.size ();

      if (n > b.capacity ())
        b.capacity (n);

      // std::vector::data() may not be available in older compilers.
      //
      if (n != 0)
        memcpy (b.data (), &v.front (), n);
    }

    //
    // default_value_traits<vector<unsigned char>, id_blob>
    //
    // std::vector has to be qualified for Sun CC.
    //
    void default_value_traits<std::vector<unsigned char>, id_blob>::
    set_image (details::buffer& b,
               size_t& n,
               bool& is_null,
               const value_type& v)
    {
      is_null = false;
      n = v.size ();

      if (n > b.capacity ())
        b.capacity (n);

      // std::vector::data() may not be available in older compilers.
      //
      if (n != 0)
        memcpy (b.data (), &v.front (), n);
    }
  }
}
