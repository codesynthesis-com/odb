// file      : odb/qt/containers/qlinked-list-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINER_QLINKED_LIST_TRAITS_HXX
#define ODB_QT_CONTAINER_QLINKED_LIST_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QtGlobal> // QT_VERSION

// QLinkedList is deprecated since Qt5 5.15 and in Qt6 it has been moved to a
// separate library.
//
#if (QT_VERSION < 0x050F00) || ODB_QT_FORCE_QLINKEDLIST

#include <QtCore/QLinkedList>

#include <odb/container-traits.hxx>

namespace odb
{
  template <typename T>
  class access::container_traits<QLinkedList<T> >
  {
  public:
    static const container_kind kind = ck_ordered;
    static const bool smart = false;

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
        f.insert (i++, *j);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      c.clear ();

      while (more)
      {
        index_type dummy;
        c.append (value_type ());
        more = f.select (dummy, c.back ());
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      index_type i (0);
      for (typename container_type::const_iterator j (c.begin ()),
             e (c.end ()); j != e; ++j)
        f.insert (i++, *j);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };
}

#endif

#include <odb/post.hxx>

#endif // ODB_QT_CONTAINER_QLINKED_LIST_TRAITS_HXX
