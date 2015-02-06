// file      : mssql/types/traits.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include "traits.hxx"

using namespace std;

namespace odb
{
  namespace mssql
  {
    void value_traits<variant, id_long_string>::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void* tmp_buf,
                    size_t tmp_capacity)
    {
      const variant& v (*static_cast<const variant*> (context));
      string str;

      switch (v.val_type)
      {
      case variant::type_int:
        {
          ostringstream os;
          os << v.int_val;

          str = "bigint ";
          str += os.str ();
          break;
        }
      case variant::type_str:
        {
          str = "varchar ";
          str += v.str_val;
          break;
        }
      }

      // Here we assume that the temoprary buffer is large enough to fit
      // the whole string in one go. If that were not the case, then we
      // would have had to chunk it.
      //
      assert (tmp_capacity >= str.size ());
      memcpy (tmp_buf, str.c_str (), str.size ());

      *buffer = tmp_buf;
      *size = str.size ();
      *chunk = chunk_one;
    }

    void value_traits<variant, id_long_string>::
    result_callback (void* context,
                     size_t*,
                     void** buffer,
                     size_t* size,
                     chunk_type chunk,
                     size_t,
                     void* tmp_buf,
                     size_t tmp_capacity)
    {
      variant& v (*static_cast<variant*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          assert (false); // The value cannot be NULL or empty.
          break;
        }
      case chunk_first:
        {
          // Use the variant's string value as a temporary buffer. If this
          // were not possible, we could have allocated one as part of
          // context.
          //
          v.str_val.clear ();

          *buffer = tmp_buf;
          *size = tmp_capacity;
          break;
        }
      case chunk_next:
        {
          v.str_val.append (static_cast<char*> (tmp_buf), *size);

          *buffer = tmp_buf;
          *size = tmp_capacity;
          break;
        }
      case chunk_last:
        {
          v.str_val.append (static_cast<char*> (tmp_buf), *size);

          // Figure out what we've got.
          //
          string::size_type p (v.str_val.find (' '));
          assert (p != string::npos); // Must have type followed by value.
          string type (v.str_val, 0, p);
          string text (v.str_val, p + 1, string::npos);

          if (type == "tinyint"  ||
              type == "smallint" ||
              type == "int"      ||
              type == "bigint")
          {
            istringstream is (text);
            is >> v.int_val;
            v.val_type = variant::type_int;
          }
          else if (type == "char" || type == "varchar")
          {
            v.str_val = text;
            v.val_type = variant::type_str;
          }
          else
            assert (false); // Unknown type.

          break;
        }
      }
    }
  }
}
