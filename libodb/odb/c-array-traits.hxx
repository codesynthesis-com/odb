// file      : odb/c-array-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_C_ARRAY_TRAITS_HXX
#define ODB_C_ARRAY_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t
#include <cassert>

#include <odb/container-traits.hxx>

namespace odb
{
  // Optional mapping of C arrays as containers. Note that this mapping is not
  // enable by default. To enable, pass the following options to the ODB
  // compiler:
  //
  // --odb-epilogue '#include <odb/c-array-traits.hxx>'
  // --hxx-prologue '#include <odb/c-array-traits.hxx>'
  //
  // Note also that the array types have to be named, for example:
  //
  // class object
  // {
  //   // composite_type values[5]; // Won't work.
  //
  //   typedef composite_type composite_array[5];
  //   composite_array values;
  // };
  //
  // Finally, this mapping is disabled for the char[N] and wchar_t[N] types
  // (they are mapped as strings by default).
  //
  template <typename V, std::size_t N>
  class access::container_traits<V[N]>
  {
  public:
    static const container_kind kind = ck_ordered;
    static const bool smart = false;

    typedef V container_type[N];

    typedef V value_type;
    typedef std::size_t index_type;

    typedef ordered_functions<index_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (index_type i (0); i < N; ++i)
        f.insert (i, c[i]);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      index_type i (0);

      for (; more && i < N; ++i)
      {
        index_type dummy;
        more = f.select (dummy, c[i]);
      }

      assert (!more && i == N);
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (index_type i (0); i < N; ++i)
        f.insert (i, c[i]);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };

  // Disable for char[N] and wchar_t[N].
  //
#ifdef ODB_COMPILER
  template <std::size_t N>
  class access::container_traits<char[N]>;

#ifdef _WIN32
  template <std::size_t N>
  class access::container_traits<wchar_t[N]>;
#endif
#endif
}

#include <odb/post.hxx>

#endif // ODB_C_ARRAY_TRAITS_HXX
