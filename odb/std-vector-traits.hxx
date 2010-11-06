// file      : odb/std-vector-traits.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_STD_VECTOR_TRAITS_HXX
#define ODB_STD_VECTOR_TRAITS_HXX

#include <odb/pre.hxx>

#include <vector>

#include <odb/container-traits.hxx>

namespace odb
{
  template <typename V>
  class access::container_traits<std::vector<V> >
  {
  public:
    static container_kind const kind = ck_ordered;

    typedef V value_type;
    typedef typename std::vector<V>::size_type index_type;
    typedef std::vector<V> container_type;

    typedef ordered_functions<index_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (index_type i (0), n (c.size ()); i < n; ++i)
        f.insert_one (i, c[i]);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        index_type dummy;
        c.push_back (value_type ());
        more = f.load_all (dummy, c.back ());
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_all ();

      for (index_type i (0), n (c.size ()); i < n; ++i)
        f.insert_one (i, c[i]);
    }

    static void
    erase (const functions& f)
    {
      f.delete_all ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_STD_VECTOR_TRAITS_HXX
