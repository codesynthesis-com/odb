// file      : odb/qt/containers/qlinked-list-traits.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINER_QLINKED_LIST_TRAITS_HXX
#define ODB_QT_CONTAINER_QLINKED_LIST_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QLinkedList>

#include <odb/container-traits.hxx>

namespace odb
{
  template <typename T>
  class access::container_traits<QLinkedList<T> >
  {
  public:
    static container_kind const kind = ck_ordered;

    typedef QLinkedList<T> container_type;

    typedef T value_type;
    typedef typename container_type::size_type index_type;

    typedef ordered_functions<index_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      index_type i (0);
      for (typename container_type::const_iterator j (c.begin ()),
             e (c.end ()); j != e; ++j)
        f.insert_one (i++, *j);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        index_type dummy;
        c.append (value_type ());
        more = f.load_all (dummy, c.back ());
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_all ();

      index_type i (0);
      for (typename container_type::const_iterator j (c.begin ()),
             e (c.end ()); j != e; ++j)
        f.insert_one (i++, *j);
    }

    static void
    erase (const functions& f)
    {
      f.delete_all ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_QT_CONTAINER_QLINKED_LIST_TRAITS_HXX
