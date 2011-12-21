// file      : odb/mssql/traits.txx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
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
    // array_long_binary_value_traits
    //

    template <typename C, std::size_t N>
    void array_long_binary_value_traits<C, N>::
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
    void array_long_binary_value_traits<C, N>::
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
