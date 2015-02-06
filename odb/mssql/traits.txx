// file      : odb/mssql/traits.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

namespace odb
{
  namespace mssql
  {
    //
    // wrapped_value_traits<W, ID, true>
    //
    template <typename W, database_type_id ID>
    void wrapped_value_traits<W, ID, true>::
    result_callback (void* context,
                     std::size_t* position,
                     void** buffer,
                     std::size_t* size,
                     chunk_type chunk,
                     std::size_t size_left,
                     void* tmp_buffer,
                     std::size_t tmp_capacity)
    {
      W& v (*static_cast<W*> (context));

      if (chunk == chunk_null)
        wtraits::set_null (v);
      else
      {
        long_callback& c (*static_cast<long_callback*> (*buffer));

        // Redirect all further calls.
        //
        vtraits::set_value (wtraits::set_ref (v),
                            c.callback.result,
                            c.context.result);

        // Forward this call.
        //
        c.callback.result (
          c.context.result,
          position,
          buffer,
          size,
          chunk,
          size_left,
          tmp_buffer,
          tmp_capacity);
      }
    }

    //
    // c_array_long_value_traits_base
    //
    template <std::size_t N>
    void c_array_long_value_traits_base<N>::
    param_callback (const void* context,
                    std::size_t*,
                    const void** buffer,
                    std::size_t* size,
                    chunk_type* chunk,
                    void*,
                    std::size_t)
    {
      // Figure out the length. We cannot use strlen since it may
      // not be 0-terminated (strnlen is not standard).
      //
      size_t n (0);
      for (; n != N && static_cast<const char*> (context)[n] != '\0'; ++n);

      *buffer = context;
      *size = n;
      *chunk = chunk_one;
    }

    template <std::size_t N>
    void c_array_long_value_traits_base<N>::
    result_callback (void* context,
                     std::size_t* position,
                     void** buffer,
                     std::size_t* size,
                     chunk_type chunk,
                     std::size_t,
                     void* tmp_buf,
                     std::size_t tmp_capacity)
    {
      char* p (static_cast<char*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          *p = '\0';
          break;
        }
      case chunk_first:
        {
          *buffer = p;
          *size = N;
          break;
        }
      case chunk_next:
        {
          // ODBC insists on appending '\0' to each chunk it returns.
          // As a result, we can get here if the last character did not
          // fit into our buffer. There could also be more data but since
          // there is no way to stop until we read all the data, dump
          // the remainder into the temporary buffer.
          //

          // Use position to indicate whether this is the first "next
          // chunk".
          //
          if (*position == 0)
            *position = 1;
          else if (*position == 1)
          {
            p[N - 1] = *static_cast<const char*> (tmp_buf);
            *position = 2;
          }

          *buffer = tmp_buf;
          *size = tmp_capacity;
          break;
        }
      case chunk_last:
        {
          if (*position == 0)
          {
            if (*size < N) // Append '\0' if there is space.
              p[*size] = '\0';
          }
          else if (*position == 1)
            p[N - 1] = *static_cast<const char*> (tmp_buf);

          break;
        }
      }
    }

    //
    // c_warray_long_value_traits_base<2>
    //
    template <std::size_t N>
    void c_warray_long_value_traits_base<N, 2>::
    param_callback (const void* context,
                    std::size_t*,
                    const void** buffer,
                    std::size_t* size,
                    chunk_type* chunk,
                    void*,
                    std::size_t)
    {
      // Figure out the length. We cannot use wcslen since it may
      // not be 0-terminated (wcsnlen is not standard).
      //
      size_t n (0);
      for (; n != N && static_cast<const wchar_t*> (context)[n] != L'\0'; ++n);

      *buffer = context;
      *size = n * 2; // In bytes.
      *chunk = chunk_one;
    }

    template <std::size_t N>
    void c_warray_long_value_traits_base<N, 2>::
    result_callback (void* context,
                     std::size_t* position,
                     void** buffer,
                     std::size_t* size,
                     chunk_type chunk,
                     std::size_t,
                     void* tmp_buf,
                     std::size_t tmp_capacity)
    {
      wchar_t* p (static_cast<wchar_t*> (context));

      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          *p = L'\0';
          break;
        }
      case chunk_first:
        {
          *buffer = p;
          *size = N * 2; // In bytes
          break;
        }
      case chunk_next:
        {
          // ODBC insists on appending '\0' to each chunk it returns.
          // As a result, we can get here if the last character did not
          // fit into our buffer. There could also be more data but since
          // there is no way to stop until we read all the data, dump
          // the remainder into the temporary buffer.
          //

          // Use position to indicate whether this is the first "next
          // chunk".
          //
          if (*position == 0)
            *position = 1;
          else if (*position == 1)
          {
            p[N - 1] = *static_cast<const wchar_t*> (tmp_buf);
            *position = 2;
          }

          *buffer = tmp_buf;
          *size = tmp_capacity;
          break;
        }
      case chunk_last:
        {
          if (*position == 0)
          {
            size_t n (*size / 2); // In wide characters.
            if (n < N) // Append '\0' if there is space.
              p[n] = L'\0';
          }
          else if (*position == 1)
            p[N - 1] = *static_cast<const wchar_t*> (tmp_buf);

          break;
        }
      }
    }

#ifndef _WIN32
    //
    // c_warray_long_value_traits_base<4>
    //
    template <std::size_t N>
    void c_warray_long_value_traits_base<N, 4>::
    param_callback (const void* context,
                    std::size_t* pos,
                    const void** buffer,
                    std::size_t* size,
                    chunk_type* chunk,
                    void* tmp_buf,
                    std::size_t tmp_capacity)
    {
      // Here we cannot just return the pointer to the underlying buffer
      // since the character sizes are different. Instead we copy the
      // data to the temporary buffer.
      //
      ucs2_char* d (static_cast<ucs2_char*> (tmp_buf));
      const wchar_t* s (static_cast<const wchar_t*> (context) + *pos);

      size_t n (0);
      tmp_capacity /= 2; // In UCS-2 character.
      for (const wchar_t* e (s + N);
           s != e && *s != L'\0' && n != tmp_capacity;
           ++n, ++s)
        d[n] = static_cast<ucs2_char> (*s);

      *buffer = d;
      *size = n * 2; // In bytes.
      *chunk = (n != tmp_capacity ? chunk_last : chunk_next);

      if (*pos == 0)
      {
        if (*chunk == chunk_last)
          *chunk = chunk_one;
        else
          *chunk = chunk_first;
      }

      *pos += n;
    }

    template <std::size_t N>
    void c_warray_long_value_traits_base<N, 4>::
    result_callback (void* context,
                     std::size_t* pos,
                     void** buffer,
                     std::size_t* size,
                     chunk_type chunk,
                     std::size_t,
                     void* tmp_buf,
                     std::size_t tmp_capacity)
    {
      wchar_t* d (static_cast<wchar_t*> (context));
      const ucs2_char* s (static_cast<const ucs2_char*> (tmp_buf));

      // Again, we cannot do direct buffer copy and have to use the
      // temporary buffer instead.
      //
      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          *d = L'\0';
          break;
        }
      case chunk_first:
        {
          break;
        }
      case chunk_next:
      case chunk_last:
        {
          // Append the data from the temporary buffer.
          //
          if (*pos < N)
          {
            *size /= 2; // In UCS-2 characters.
            size_t n (N - *pos);
            n = *size < n ? *size : n;

            wstring_functions<>::assign (d + *pos, s, n);
            *pos += n;

            if (*pos < N) // Append '\0' if there is space.
              d[*pos] = L'\0';
          }

          break;
        }
      }

      if (chunk == chunk_first || chunk == chunk_next)
      {
        *buffer = tmp_buf;
        *size = tmp_capacity;
      }
    }
#endif

    //
    // c_array_long_binary_value_traits
    //
    template <typename C, std::size_t N>
    void c_array_long_binary_value_traits<C, N>::
    param_callback (const void* context,
                    std::size_t*,
                    const void** buffer,
                    std::size_t* size,
                    chunk_type* chunk,
                    void*,
                    std::size_t)
    {
      *buffer = context;
      *size = N;
      *chunk = chunk_one;
    }

    template <typename C, std::size_t N>
    void c_array_long_binary_value_traits<C, N>::
    result_callback (void* context,
                     std::size_t*,
                     void** buffer,
                     std::size_t* size,
                     chunk_type chunk,
                     std::size_t size_left,
                     void* tmp_buf,
                     std::size_t tmp_capacity)
    {
      // The code is similar to the vector<char> specialization.
      //
      switch (chunk)
      {
      case chunk_null:
      case chunk_one:
        {
          std::memset (context, 0, N);
          break;
        }
      case chunk_first:
        {
          assert (size_left != 0);

          *buffer = context;
          *size = size_left < N ? size_left : N;
          break;
        }
      case chunk_next:
        {
          // We can get here if total size is greater than N. There is
          // no way to stop until we read all the data, so dump the
          // remainder into the temporary buffer.
          //
          *buffer = tmp_buf;
          *size = tmp_capacity;
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
