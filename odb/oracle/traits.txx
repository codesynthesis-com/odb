// file      : odb/oracle/traits.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
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
      *p = chunk_one;
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
      *p = chunk_one;
      *s = static_cast<ub4> (N);
      *b = c;
      return true;
    }

#ifdef ODB_CXX11
    //
    // default_value_traits<std::array<char, N>, id_blob>
    //

    template <std::size_t N>
    bool default_value_traits<std::array<char, N>, id_blob>::
    result_callback (void* c, ub4* position, void* b, ub4 s, chunk_position)
    {
      ub4 n (*position + s < N ? s : N - *position);
      std::memcpy (static_cast<char*> (c) + *position, b, n);
      *position += n;
      return true;
    }

    template <std::size_t N>
    bool default_value_traits<std::array<char, N>, id_blob>::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      *p = chunk_one;
      *s = static_cast<ub4> (N);
      *b = c;
      return true;
    }

    //
    // default_value_traits<std::array<unsigned char, N>, id_blob>
    //

    template <std::size_t N>
    bool default_value_traits<std::array<unsigned char, N>, id_blob>::
    result_callback (void* c, ub4* position, void* b, ub4 s, chunk_position)
    {
      ub4 n (*position + s < N ? s : N - *position);
      std::memcpy (static_cast<unsigned char*> (c) + *position, b, n);
      *position += n;
      return true;
    }

    template <std::size_t N>
    bool default_value_traits<std::array<unsigned char, N>, id_blob>::
    param_callback (const void* c,
                    ub4*,
                    const void** b,
                    ub4* s,
                    chunk_position* p,
                    void*,
                    ub4)
    {
      *p = chunk_one;
      *s = static_cast<ub4> (N);
      *b = c;
      return true;
    }
#endif
  }
}
