// file      : odb/oracle/traits.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memcpy, std::strlen
#include <cassert>

#include <odb/oracle/traits.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    //
    // string_value_traits
    //

    void string_value_traits::
    set_image (char* b,
               size_t c,
               size_t& n,
               bool& is_null,
               const string& v)
    {
      is_null = false;
      n = v.size ();

      assert (n <= c);

      if (n != 0 && n <= c)
        memcpy (b, v.c_str (), n);
    }

    //
    // c_string_value_traits
    //

    void c_string_value_traits::
    set_image (char* b,
               size_t c,
               size_t& n,
               bool& is_null,
               const char* v)
    {
      is_null = false;
      n = strlen (v);

      assert (n <= c);

      if (n != 0 && n <= c)
        memcpy (b, v, n);
    }

    //
    // default_value_traits<vector<char>, id_raw>
    //

    void default_value_traits<vector<char>, id_raw>::
    set_image (char* b,
               size_t c,
               size_t& n,
               bool& is_null,
               const value_type& v)
    {
      is_null = false;
      n = v.size ();

      assert (n <= c);

      // std::vector::data() may not be available in older compilers.
      //
      if (n != 0)
        memcpy (b, &v.front (), n);
    }

    //
    // default_value_traits<vector<unsigned char>, id_raw>
    //

    void default_value_traits<vector<unsigned char>, id_raw>::
    set_image (char* b,
               size_t c,
               size_t& n,
               bool& is_null,
               const value_type& v)
    {
      is_null = false;
      n = v.size ();

      assert (n <= c);

      // std::vector::data() may not be available in older compilers.
      //
      if (n != 0)
        memcpy (b, &v.front (), n);
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
      case one_chunk:
      case first_chunk:
        {
          v.clear ();

          // Falling through.
        }
      case next_chunk:
      case last_chunk:
        {
          v.append (static_cast<char*> (b), s);
          break;
        }
      }

      return true;
    }

    bool string_lob_value_traits::
    param_callback (const void* ctx,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const string& v (*static_cast<const string*> (ctx));

      *p = one_chunk;
      *s = static_cast<ub4> (v.size ());
      *b = v.c_str ();

      return true;
    }

    //
    // c_string_lob_value_traits
    //

    bool c_string_lob_value_traits::
    param_callback (const void* ctx,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const char* v (static_cast<const char*> (ctx));

      *p = one_chunk;
      *s = static_cast<ub4> (strlen (v));
      *b = v;

      return true;
    }

    //
    // default_value_traits<std::vector<char>, id_blob>
    //

    bool default_value_traits<std::vector<char>, id_blob>::
    result_callback (void* c, ub4*, void* b, ub4 s, chunk_position p)
    {
      value_type& v (*static_cast<value_type*> (c));

      switch (p)
      {
      case one_chunk:
      case first_chunk:
        {
          v.clear ();

          // Falling through.
        }
      case next_chunk:
      case last_chunk:
        {
          char* cb (static_cast<char*> (b));
          v.insert (v.end (), cb, cb + s);

          break;
        }
      }

      return true;
    }

    bool default_value_traits<std::vector<char>, id_blob>::
    param_callback (const void* ctx,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const value_type& v (*static_cast<const value_type*> (ctx));

      *p = one_chunk;
      *s = static_cast<ub4> (v.size ());
      *b = &v.front ();

      return true;
    }

    //
    // default_value_traits<std::vector<unsigned char>, id_blob>
    //

    bool default_value_traits<std::vector<unsigned char>, id_blob>::
    result_callback (void* c, ub4*, void* b, ub4 s, chunk_position p)
    {
      value_type& v (*static_cast<value_type*> (c));

      switch (p)
      {
      case one_chunk:
      case first_chunk:
        {
          v.clear ();

          // Falling through.
        }
      case next_chunk:
      case last_chunk:
        {
          unsigned char* cb (static_cast<unsigned char*> (b));
          v.insert (v.end (), cb, cb + s);

          break;
        }
      }

      return true;
    }

    bool default_value_traits<std::vector<unsigned char>, id_blob>::
    param_callback (const void* ctx,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const value_type& v (*static_cast<const value_type*> (ctx));

      *p = one_chunk;
      *s = static_cast<ub4> (v.size ());
      *b = &v.front ();

      return true;
    }
  }
}
