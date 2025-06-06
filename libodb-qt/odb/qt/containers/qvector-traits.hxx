// file      : odb/qt/containers/qvector-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINER_QVECTOR_TRAITS_HXX
#define ODB_QT_CONTAINER_QVECTOR_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QtGlobal> // QT_VERSION

// In Qt 6 QVector is an alias for QList.
//
#if QT_VERSION >= 0x060000
#include <odb/qt/containers/qlist-traits.hxx>
#else

#include <QtCore/QVector>

#include <odb/container-traits.hxx>

namespace odb
{
  template <typename T>
  class access::container_traits<QVector<T> >
  {
  public:
    static const container_kind kind = ck_ordered;
    static const bool smart = false;

    typedef QVector<T> container_type;

    typedef T value_type;
    typedef typename container_type::size_type index_type;

    typedef ordered_functions<index_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      // Index based access is just as fast as iterator based access for
      // QVector.
      //
      for (index_type i (0), n (c.size ()); i < n; ++i)
        f.insert (i, c[i]);
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

      for (index_type i (0), n (c.size ()); i < n; ++i)
        f.insert (i, c[i]);
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

#endif // ODB_QT_CONTAINER_QVECTOR_TRAITS_HXX
