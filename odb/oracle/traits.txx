// file      : odb/oracle/traits.txx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    //
    // default_value_traits<char[N], id_blob>
    //

    template <std::size_t N>
    bool default_value_traits<char[N], id_blob>::
    result_callback (void* c, ub4* position, void* b, ub4 s, chunk_position)
    {
      ub4 n (*position + s < N ? s : N - *position);
      std::memcpy (static_cast<char*> (c) + *position, b, n);
      *position += n;
      return true;
    }

    template <std::size_t N>
    bool default_value_traits<char[N], id_blob>::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      *p = one_chunk;
      *s = static_cast<ub4> (N);
      *b = c;
      return true;
    }

    //
    // default_value_traits<unsigned char[N], id_blob>
    //

    template <std::size_t N>
    bool default_value_traits<unsigned char[N], id_blob>::
    result_callback (void* c, ub4* position, void* b, ub4 s, chunk_position)
    {
      ub4 n (*position + s < N ? s : N - *position);
      std::memcpy (static_cast<unsigned char*> (c) + *position, b, n);
      *position += n;
      return true;
    }

    template <std::size_t N>
    bool default_value_traits<unsigned char[N], id_blob>::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      *p = one_chunk;
      *s = static_cast<ub4> (N);
      *b = c;
      return true;
    }
  }
}
