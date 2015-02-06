// file      : odb/qt/containers/qmap-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINER_QMAP_TRAITS_HXX
#define ODB_QT_CONTAINER_QMAP_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QMap>
#include <QtCore/QMultiMap>

#include <odb/container-traits.hxx>

namespace odb
{
  template <typename Key, typename T>
  class access::container_traits<QMap<Key, T> >
  {
  public:
    static const container_kind kind = ck_map;
    static const bool smart = false;

    typedef QMap<Key, T> container_type;

    typedef Key key_type;
    typedef T value_type;

    typedef map_functions<key_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i.key (), i.value ());
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
        c.insert (k, v); //@@ Use std::move in C++11.
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i.key (), i.value ());
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };
  // @@ QMultiMap guarantees elements to be stored in reverse order of
  // insertion. The current implementation of the generated code does
  // not guarantee this.
  //
  template <typename Key, typename T>
  class access::container_traits<QMultiMap<Key, T> >
  {
  public:
    static const container_kind kind = ck_multimap;
    static const bool smart = false;

    typedef QMultiMap<Key, T> container_type;

    typedef Key key_type;
    typedef T value_type;

    typedef map_functions<key_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i.key (), i.value ());
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
        c.insert (k, v); //@@ Use std::move in C++11.
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      for (typename container_type::const_iterator i (c.begin ()),
             e (c.end ()); i != e; ++i)
        f.insert (i.key (), i.value ());
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_QT_CONTAINER_QMAP_TRAITS_HXX
