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
    // string_lob_value_traits
    //

    bool string_lob_value_traits::
    param_callback (void* ctx,
                    ub4* pos_ctx,
                    void** b,
                    ub4* s,
                    chunk_position* p,
                    void* temp_b,
                    ub4 cap)
    {
      const string& v (*reinterpret_cast<string*> (ctx));

      // @@ We rely on *pos_ctx == 0 for the first call. Make sure that this is
      // set by the generated code, and update comment in oracle-types.hxx
      // specifying that this is so.
      //
      *s = static_cast<ub4> (v.size ()) - *pos_ctx;

      if (*s <= cap)
        *p = *pos_ctx == 0 ? one_chunk : last_chunk;
      else
      {
        *p = *pos_ctx == 0 ? first_chunk : next_chunk;
        *s = cap;
        *pos_ctx += *s;
      }

      *b = temp_b;
      memcpy (temp_b, v.c_str () + *pos_ctx, *s);

      return true;
    }

    //
    // c_string_lob_value_traits
    //

    bool c_string_lob_value_traits::
    param_callback (void* ctx,
                    ub4* pos_ctx,
                    void** b,
                    ub4* s,
                    chunk_position* p,
                    void* temp_b,
                    ub4 cap)
    {
      const char* v (reinterpret_cast<char*> (ctx));

      // @@ We rely on *pos_ctx == 0 for the first call. Make sure that this is
      // set by the generated code, and update comment in oracle-types.hxx
      // specifying that this is so.
      //
      *s = static_cast<ub4> (strlen (v)) - *pos_ctx;

      if (*s <= cap)
        *p = *pos_ctx == 0 ? one_chunk : last_chunk;
      else
      {
        *p = *pos_ctx == 0 ? first_chunk : next_chunk;
        *s = cap;
        *pos_ctx += *s;
      }

      *b = temp_b;
      memcpy (temp_b, v + *pos_ctx, *s);

      return true;
    }

    //
    // default_value_traits<std::vector<char>, id_blob>
    //
    bool default_value_traits<std::vector<char>, id_blob>::
    param_callback (void* ctx,
                    ub4* pos_ctx,
                    void** b,
                    ub4* s,
                    chunk_position* p,
                    void* temp_b,
                    ub4 cap)
    {
      const value_type& v (*reinterpret_cast<value_type*> (ctx));

      // @@ We rely on *position_context == 0 for the first call. Make sure
      // that this is set by the generated code and update the comment in
      // oracle-types.hxx specifying that this is so.
      //
      *s = static_cast<ub4> (v.size ()) - *pos_ctx;

      if (*s <= cap)
        *p = *pos_ctx == 0 ? one_chunk : last_chunk;
      else
      {
        *p = *pos_ctx == 0 ? first_chunk : next_chunk;
        *s = cap;
        *pos_ctx += *s;
      }

      *b = temp_b;
      memcpy (temp_b, &v[*pos_ctx], *s);

      return true;
    }
  }
}
