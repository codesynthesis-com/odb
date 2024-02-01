// file      : odb/boost/unordered/container-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_UNORDERED_CONTAINER_TRAITS_HXX
#define ODB_BOOST_UNORDERED_CONTAINER_TRAITS_HXX

#include <boost/version.hpp>

// Unordered containers are available since 1.36.0.
//
#if BOOST_VERSION >= 103600

#include <odb/pre.hxx>

#include <utility> // std::move

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include <odb/container-traits.hxx>
#include <odb/details/config.hxx>   // ODB_CXX11

namespace odb
{
  // unordered_set
  //
  template <typename V, typename H, typename P, typename A>
  class access::container_traits< ::boost::unordered_set<V, H, P, A> >
  {
  public:
    static const container_kind kind = ck_set;
    static const bool smart = false;

    typedef ::boost::unordered_set<V, H, P, A> container_type;
    typedef V value_type;

    typedef set_functions<value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (*i);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        value_type v;
        more = f.select (v);
#ifdef ODB_CXX11
        c.insert (std::move (v));
#else
        c.insert (v);
#endif
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (*i);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };

  // unordered_multiset
  //
  // @@ Does multiset preserve insertion order of equal elements? The
  // current implementation in the generated code does not guarantee
  // this.
  //
  template <typename V, typename H, typename P, typename A>
  class access::container_traits< ::boost::unordered_multiset<V, H, P, A> >
  {
  public:
    static const container_kind kind = ck_multiset;
    static const bool smart = false;

    typedef ::boost::unordered_multiset<V, H, P, A> container_type;
    typedef V value_type;

    typedef set_functions<value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (*i);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        value_type v;
        more = f.select (v);
#ifdef ODB_CXX11
        c.insert (std::move (v));
#else
        c.insert (v);
#endif
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (*i);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };

  // unordered_map
  //
  template <typename K, typename V, typename H, typename P, typename A>
  class access::container_traits< ::boost::unordered_map<K, V, H, P, A> >
  {
  public:
    static const container_kind kind = ck_map;
    static const bool smart = false;

    typedef ::boost::unordered_map<K, V, H, P, A> container_type;

    typedef K key_type;
    typedef V value_type;
    typedef typename container_type::value_type pair_type;

    typedef map_functions<key_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i->first, i->second);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        key_type k;
        value_type v;
        more = f.select (k, v);

#ifdef ODB_CXX11
        c.insert (pair_type (std::move (k), std::move (v)));
#else
        c.insert (pair_type (k, v));
#endif
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i->first, i->second);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };

  // unordered_multimap
  //
  // @@ Does multimap preserve insertion order of equal elements? The
  // current implementation in the generated code does not guarantee
  // this.
  //
  template <typename K, typename V, typename H, typename P, typename A>
  class access::container_traits< ::boost::unordered_multimap<K, V, H, P, A> >
  {
  public:
    static const container_kind kind = ck_multimap;
    static const bool smart = false;

    typedef ::boost::unordered_multimap<K, V, H, P, A> container_type;

    typedef K key_type;
    typedef V value_type;
    typedef typename container_type::value_type pair_type;

    typedef map_functions<key_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i->first, i->second);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        key_type k;
        value_type v;
        more = f.select (k, v);

#ifdef ODB_CXX11
        c.insert (pair_type (std::move (k), std::move (v)));
#else
        c.insert (pair_type (k, v));
#endif
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i->first, i->second);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };
}

#include <odb/post.hxx>

#endif // BOOST_VERSION
#endif // ODB_BOOST_UNORDERED_CONTAINER_TRAITS_HXX
