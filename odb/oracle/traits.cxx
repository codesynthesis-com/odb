// file      : odb/oracle/traits.cxx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/oracle/traits.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
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
    // c_string_lob_value_traits
    //

    bool c_string_lob_value_traits::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      const char* v (static_cast<const char*> (c));

      *p = chunk_one;
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
