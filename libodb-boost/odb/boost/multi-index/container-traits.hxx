// file      : odb/boost/multi-index/container-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_MULTI_INDEX_CONTAINER_TRAITS_HXX
#define ODB_BOOST_MULTI_INDEX_CONTAINER_TRAITS_HXX

#include <boost/version.hpp>

// Multi-index container is available since 1.32.0.
//
#if BOOST_VERSION >= 103200

#include <odb/pre.hxx>

#include <utility> // std::move

#include <boost/mpl/find_if.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/begin_end.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include <odb/container-traits.hxx>
#include <odb/details/config.hxx>   // ODB_CXX11

namespace odb
{
  // Multi-index container can be ordered or set. Whether it is ordered
  // depends on whether it has any sequenced or random_access indexes.
  // If it is ordered, then we need to use one of these indexes to store
  // elements in a specific order. Otherwise, we can use the first index
  // since the order of elements in the database is not important. Note
  // that there is a terminology clash between ODB and Boost multi-index.
  // What ODB calls ordered containers, multi-index calls sequenced and
  // random_access. And what ODB calls set containers, multi-index calls
  // ordered and hashed.
  //

  // Test whether index is ordered.
  //
  template <typename I>
  struct multi_index_ordered
  {
    static const bool value = false;
  };

  template <typename T>
  struct multi_index_ordered< ::boost::multi_index::sequenced<T> >
  {
    static const bool value = true;
  };

  template <typename T>
  struct multi_index_ordered< ::boost::multi_index::random_access<T> >
  {
    static const bool value = true;
  };

  // Get the index of the first ordered sub-index or -1 if none exists.
  //
  template <typename B, typename I, typename E>
  struct multi_index_ordered_index
  {
    static const int value = ::boost::mpl::distance<B, I>::value;
  };

  template <typename B, typename E>
  struct multi_index_ordered_index<B, E, E>
  {
    static const int value = -1;
  };

  template <typename V,
            typename ISP,
            typename A,
            int N =
            multi_index_ordered_index<
              typename ::boost::mpl::begin<ISP>::type,
              typename ::boost::mpl::find_if<
                ISP, multi_index_ordered< ::boost::mpl::_1 > >::type,
              typename ::boost::mpl::end<ISP>::type>::value>
  class multi_index_traits
  {
  public:
    static const container_kind kind = ck_ordered;
    static const bool smart = false;

    typedef ::boost::multi_index_container<V, ISP, A> container_type;
    typedef typename container_type::template nth_index<N>::type
    ordered_container_type;

    typedef V value_type;
    typedef typename ordered_container_type::size_type index_type;

    typedef ordered_functions<index_type, value_type> functions;

  public:
    static void
    persist (const container_type& c, const functions& f)
    {
      const ordered_container_type& oc (c.template get<N> ());
      index_type i (0);
      for (typename ordered_container_type::const_iterator j (oc.begin ()),
             e (oc.end ()); j != e; ++j)
        f.insert (i++, *j);
    }

    static void
    load (container_type& c, bool more, const functions& f)
    {
      ordered_container_type& oc (c.template get<N> ());
      oc.clear ();

      while (more)
      {
        index_type dummy;
        value_type v;
        more = f.select (dummy, v);
#ifdef ODB_CXX11
        oc.push_back (std::move (v));
#else
        oc.push_back (v);
#endif
      }
    }

    static void
    update (const container_type& c, const functions& f)
    {
      f.delete_ ();

      const ordered_container_type& oc (c.template get<N> ());
      index_type i (0);
      for (typename ordered_container_type::const_iterator j (oc.begin ()),
             e (oc.end ()); j != e; ++j)
        f.insert (i++, *j);
    }

    static void
    erase (const functions& f)
    {
      f.delete_ ();
    }
  };

  // Set specialization.
  //
  template <typename V, typename ISP, typename A>
  class multi_index_traits<V, ISP, A, -1>
  {
  public:
    static const container_kind kind = ck_set;
    static const bool smart = false;

    typedef ::boost::multi_index_container<V, ISP, A> container_type;
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

  template <typename V, typename ISP, typename A>
  class access::container_traits< ::boost::multi_index_container<V, ISP, A> >
    : public multi_index_traits<V, ISP, A>
  {
  };
}

#include <odb/post.hxx>

#endif // BOOST_VERSION
#endif // ODB_BOOST_MULTI_INDEX_CONTAINER_TRAITS_HXX
