// file      : odb/mssql/traits.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <odb/mssql/traits.hxx>

using namespace std;

namespace odb
{
  namespace mssql
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
    // default_value_traits<std::string, id_long_string>
    //
    void default_value_traits<string, id_long_string>::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void*,
                    size_t)
    {
      const string& str (*static_cast<const string*> (context));

      *buffer = str.c_str ();
      *size = str.size ();
      *chunk = chunk_one;
    }

    void default_value_traits<string, id_long_string>::
    result_callback (void* context,
                     size_t* position,
                     void** buffer,
                     size_t* size,
                     chunk_type chunk,
                     size_t size_left,
                     void* tmp_buf,
                     size_t tmp_capacity)
    {
      string& str (*static_cast<string*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          str.clear ();
          break;
        }
      case chunk_first:
        {
          // If the total size is available, then pre-allocate the string
          // and copy the data directly into its buffer in one go. While
          // this kind of direct modification of the std::string buffer
          // is not sanctioned by the standard, this is known to work
          // with all the implementations we care to support. We just
          // need to make sure the underlying buffer is not shared with
          // any other instance if the implementation uses COW.
          //
          if (size_left != 0)
          {
            size_left++; // One extra for the null terminator.

            if (str.size () != size_left)
              str.resize (size_left);
            else
              str[0] = '\0'; // Force copy in a COW implementation.

            *buffer = const_cast<char*> (str.c_str ());
            *size = size_left;
            *position = 0; // Indicator.
          }
          else
          {
            // If the total size is not available, do the short string
            // optimization by first returning a small temporary buffer.
            // If the data fits, then we copy it over to the string and
            // thus get the precise buffer allocation. If the data does
            // not fit, then we resort to the exponential buffer growth.
            //
            *buffer = tmp_buf;
            *size = tmp_capacity > 128 ? 128 : tmp_capacity;
            *position = 1; // Indicator.
          }

          break;
        }
      case chunk_next:
        {
          // We should only end up here if the short string optimization
          // didn't work out.
          //
          assert (*position != 0);

          if (*position == 1)
          {
            // First chunk_next call. There is some data in the temp
            // buffer which we need to copy over.
            //
            str.reserve (256);
            str.assign (static_cast<char*> (tmp_buf), *size);
            *position = *size; // Size of actual data in str, which got to be
                               // greater than 128, or we would have gotten
                               // chunk_last.
            str.resize (256);
          }
          else
          {
            // Subsequent chunk_next call. Double the buffer and continue.
            //
            *position += *size;
            str.resize (str.size () * 2);
          }

          *buffer = const_cast<char*> (str.c_str ()) + *position;
          *size = str.size () - *position;
          break;
        }
      case chunk_last:
        {
          if (*position == 0)
            str.resize (*size);
          else if (*position == 1)
            // Short string optimization worked out. There is some data
            // in the temp buffer which we need to copy over.
            //
            str.assign (static_cast<char*> (tmp_buf), *size);
          else
            str.resize (*position + *size);

          break;
        }
      }
    }

    //
    // c_long_string_value_traits
    //
    void c_string_long_value_traits::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void*,
                    size_t)
    {
      *buffer = context;
      *size = strlen (static_cast<const char*> (context));
      *chunk = chunk_one;
    }

    //
    // c_warray_value_traits_base
    //
    void c_warray_value_traits_base::
    set_value (wchar_t* const& v,
               const ucs2_char* b,
               size_t n,
               bool is_null,
               size_t N)
    {
      if (!is_null)
      {
        n = n < N ? n : N;

        if (n != 0)
          functions::assign (v, b, n);
      }
      else
        n = 0;

      if (n != N) // Append '\0' if there is space.
        v[n] = L'\0';
    }

    void c_warray_value_traits_base::
    set_image (ucs2_char* b,
               size_t c,
               size_t& n,
               bool& is_null,
               const wchar_t* v,
               size_t N)
    {
      is_null = false;

      // Figure out the length. We cannot use wcslen since it may
      // not be 0-terminated (wcsnlen is not standard).
      //
      for (n = 0; n != N && v[n] != L'\0'; ++n) ;

      if (n > c)
        n = c;

      if (n != 0)
        functions::assign (b, v, n);
    }

    //
    // wstring_long_value_traits<2>
    //
    void wstring_long_value_traits<2>::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void*,
                    size_t)
    {
      const wstring& str (*static_cast<const wstring*> (context));

      *buffer = str.c_str ();
      *size = str.size () * 2; // In bytes.
      *chunk = chunk_one;
    }

    void wstring_long_value_traits<2>::
    result_callback (void* context,
                     size_t*,
                     void** buffer,
                     size_t* size,
                     chunk_type chunk,
                     size_t size_left,
                     void*,
                     size_t)
    {
      wstring& str (*static_cast<wstring*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          str.clear ();
          break;
        }
      case chunk_first:
        {
          // The Native Client ODBC driver seems to always be able to
          // return the total size for national character strings. This
          // makes things simple and efficient.
          //
          assert (size_left != 0);

          size_left /= 2; // Convert to characters.
          size_left++;    // One extra for the null terminator.

          if (str.size () != size_left)
            str.resize (size_left);
          else
            str[0] = L'\0'; // Force copy in a COW implementation.

          *buffer = const_cast<wchar_t*> (str.c_str ());
          *size = size_left * 2; // In bytes.
          break;
        }
      case chunk_next:
        {
          // We should never get here.
          //
          assert (false);
          break;
        }
      case chunk_last:
        {
          str.resize (*size / 2); // Get rid of the null terminator.
          break;
        }
      }
    }

    //
    // wstring_long_value_traits<4>
    //
#ifndef _WIN32
    void wstring_long_value_traits<4>::
    param_callback (const void* context,
                    size_t* position,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void* tmp_buf,
                    size_t tmp_capacity)
    {
      const wstring& str (*static_cast<const wstring*> (context));

      // Here we cannot just return the pointer to the underlying buffer
      // since the character sizes are different. Instead we copy the
      // data to the temporary buffer.
      //
      *buffer = tmp_buf;
      *size = str.size () - *position; // In UCS-2 characters.

      if (*size > tmp_capacity / 2)
      {
        *size = tmp_capacity / 2;
        *chunk = chunk_next;
      }
      else
        *chunk = chunk_last;

      wstring_functions<>::assign (static_cast<ucs2_char*> (tmp_buf),
                                   str.c_str () + *position,
                                   *size);
      if (*position == 0)
      {
        if (*chunk == chunk_last)
          *chunk = chunk_one;
        else
          *chunk = chunk_first;
      }

      *position += *size;
      *size *= 2; // Convert to bytes.
    }

    void wstring_long_value_traits<4>::
    result_callback (void* context,
                     size_t*,
                     void** buffer,
                     size_t* size,
                     chunk_type chunk,
                     size_t size_left,
                     void* tmp_buf,
                     size_t tmp_capacity)
    {
      wstring& str (*static_cast<wstring*> (context));

      // Again, we cannot do direct buffer copy and have to use the
      // temporary buffer instead.
      //
      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          str.clear ();
          break;
        }
      case chunk_first:
        {
          // The Native Client ODBC driver seems to always be able to
          // return the total size for national character strings. Use
          // this to reserve enough space in the string.
          //
          assert (size_left != 0);
          str.reserve (size_left / 2);
          break;
        }
      case chunk_next:
      case chunk_last:
        {
          // Append the data from the temporary buffer.
          //
          ucs2_char* p (static_cast<ucs2_char*> (tmp_buf));
          str.append (p, p + *size / 2);
          break;
        }
      }

      if (chunk == chunk_first || chunk == chunk_next)
      {
        *buffer = tmp_buf;
        *size = tmp_capacity;
      }
    }
#endif // _WIN32

    //
    // c_wstring_long_value_traits<2>
    //
    void c_wstring_long_value_traits<2>::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void*,
                    size_t)
    {
      *buffer = context;
      *size = wcslen (static_cast<const wchar_t*> (context)) * 2; // In bytes.
      *chunk = chunk_one;
    }

    //
    // c_wstring_long_value_traits<4>
    //
#ifndef _WIN32
    void c_wstring_long_value_traits<4>::
    param_callback (const void* context,
                    size_t* position,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void* tmp_buf,
                    size_t tmp_capacity)
    {
      const wchar_t* str (static_cast<const wchar_t*> (context));

      // Here we cannot just return the pointer to the buffer since the
      // character sizes are different. Instead we copy the data to the
      // temporary buffer.
      //
      *buffer = tmp_buf;
      *size = wcslen (str) - *position; // In UCS-2 characters.

      if (*size > tmp_capacity / 2)
      {
        *size = tmp_capacity / 2;
        *chunk = chunk_next;
      }
      else
        *chunk = chunk_last;

      wstring_functions<>::assign (static_cast<ucs2_char*> (tmp_buf),
                                   str + *position,
                                   *size);
      if (*position == 0)
      {
        if (*chunk == chunk_last)
          *chunk = chunk_one;
        else
          *chunk = chunk_first;
      }

      *position += *size;
      *size *= 2; // Convert to bytes.
    }
#endif // _WIN32

    //
    // default_value_traits<std::vector<char>, id_long_binary>
    //
    // std::vector has to be qualified for Sun CC.
    //
    void default_value_traits<std::vector<char>, id_long_binary>::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void*,
                    size_t)
    {
      const value_type& v (*static_cast<const value_type*> (context));

      *buffer = &v.front ();
      *size = v.size ();
      *chunk = chunk_one;
    }

    void default_value_traits<std::vector<char>, id_long_binary>::
    result_callback (void* context,
                     size_t*,
                     void** buffer,
                     size_t* size,
                     chunk_type chunk,
                     size_t size_left,
                     void*,
                     size_t)
    {
      value_type& v (*static_cast<value_type*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          v.clear ();
          break;
        }
      case chunk_first:
        {
          // The Native Client ODBC driver seems to always be able to
          // return the total size. This makes things simple and
          // efficient.
          //
          assert (size_left != 0);

          v.resize (size_left);
          *buffer = &v.front ();
          *size = size_left;
          break;
        }
      case chunk_next:
        {
          // We should never get here.
          //
          assert (false);
          break;
        }
      case chunk_last:
        {
          // Nothing to do here. The vector is already of the correct size
          // and should contain the data.
          break;
        }
      }
    }

    //
    // default_value_traits<std::vector<unsigned char>, id_long_binary>
    //
    // std::vector has to be qualified for Sun CC.
    //
    void default_value_traits<std::vector<unsigned char>, id_long_binary>::
    param_callback (const void* context,
                    size_t*,
                    const void** buffer,
                    size_t* size,
                    chunk_type* chunk,
                    void*,
                    size_t)
    {
      const value_type& v (*static_cast<const value_type*> (context));

      *buffer = &v.front ();
      *size = v.size ();
      *chunk = chunk_one;
    }

    void default_value_traits<std::vector<unsigned char>, id_long_binary>::
    result_callback (void* context,
                     size_t*,
                     void** buffer,
                     size_t* size,
                     chunk_type chunk,
                     size_t size_left,
                     void*,
                     size_t)
    {
      // The code is exactly the same as in the vector<char> specialization.
      //
      value_type& v (*static_cast<value_type*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          v.clear ();
          break;
        }
      case chunk_first:
        {
          assert (size_left != 0);

          v.resize (size_left);
          *buffer = &v.front ();
          *size = size_left;
          break;
        }
      case chunk_next:
        {
          assert (false);
          break;
        }
      case chunk_last:
        {
          break;
        }
      }
    }
  }
}
