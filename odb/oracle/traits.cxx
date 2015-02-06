// file      : odb/oracle/traits.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/oracle/traits.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    //
    // c_array_value_traits_base
    //
    void c_array_value_traits_base::
    set_value (char* const& v,
               const char* b,
               size_t n,
               bool is_null,
               size_t N)
    {
      if (!is_null)
      {
        n = n < N ? n : N;

        if (n != 0)
          memcpy (v, b, n);
      }
      else
        n = 0;

      if (n != N) // Append '\0' if there is space.
        v[n] = '\0';
    }

    void c_array_value_traits_base::
    set_image (char* b,
               size_t c,
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

      if (n > c)
        n = c;

      if (n != 0)
        memcpy (b, v, n);
    }

    //
    // string_lob_value_traits
    //
    bool string_lob_value_traits::
    result_callback (void* c, ub4*, void* b, ub4 s, chunk_position p)
    {
      string& v (*static_cast<string*> (c));

      switch (p)
      {
      case chunk_one:
      case chunk_first:
        {
          v.clear ();

          // Falling through.
        }
      case chunk_next:
      case chunk_last:
        {
          v.append (static_cast<char*> (b), s);
          break;
        }
      }

      return true;
    }

    bool string_lob_value_traits::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const string& v (*static_cast<const string*> (c));

      *p = chunk_one;
      *s = static_cast<ub4> (v.size ());
      *b = v.c_str ();

      return true;
    }

    //
    // default_value_traits<std::vector<char>, id_blob>
    //
    // std::vector has to be qualified for Sun CC.
    //
    bool default_value_traits<std::vector<char>, id_blob>::
    result_callback (void* c, ub4*, void* b, ub4 s, chunk_position p)
    {
      value_type& v (*static_cast<value_type*> (c));

      switch (p)
      {
      case chunk_one:
      case chunk_first:
        {
          v.clear ();

          // Falling through.
        }
      case chunk_next:
      case chunk_last:
        {
          char* cb (static_cast<char*> (b));
          v.insert (v.end (), cb, cb + s);

          break;
        }
      }

      return true;
    }

    bool default_value_traits<std::vector<char>, id_blob>::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const value_type& v (*static_cast<const value_type*> (c));

      *p = chunk_one;
      *s = static_cast<ub4> (v.size ());
      *b = &v.front ();

      return true;
    }

    //
    // default_value_traits<std::vector<unsigned char>, id_blob>
    //
    // std::vector has to be qualified for Sun CC.
    //
    bool default_value_traits<std::vector<unsigned char>, id_blob>::
    result_callback (void* c, ub4*, void* b, ub4 s, chunk_position p)
    {
      value_type& v (*static_cast<value_type*> (c));

      switch (p)
      {
      case chunk_one:
      case chunk_first:
        {
          v.clear ();

          // Falling through.
        }
      case chunk_next:
      case chunk_last:
        {
          unsigned char* cb (static_cast<unsigned char*> (b));
          v.insert (v.end (), cb, cb + s);

          break;
        }
      }

      return true;
    }

    bool default_value_traits<std::vector<unsigned char>, id_blob>::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const value_type& v (*static_cast<const value_type*> (c));

      *p = chunk_one;
      *s = static_cast<ub4> (v.size ());
      *b = &v.front ();

      return true;
    }
  }
}
