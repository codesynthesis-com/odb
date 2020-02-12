// file      : odb/boost/smart-ptr/lazy-pointer-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_SMART_PTR_LAZY_POINTER_TRAITS_HXX
#define ODB_BOOST_SMART_PTR_LAZY_POINTER_TRAITS_HXX

#include <odb/pre.hxx>

#include <odb/pointer-traits.hxx>
#include <odb/boost/smart-ptr/lazy-ptr.hxx>

namespace odb
{
  template <typename T>
  class pointer_traits<boost::lazy_shared_ptr<T> >
  {
  public:
    static const pointer_kind kind = pk_shared;
    static const bool lazy = true;

    typedef T element_type;
    typedef boost::lazy_shared_ptr<element_type> pointer_type;
    typedef ::boost::shared_ptr<element_type> eager_pointer_type;

    static bool
    null_ptr (const pointer_type& p)
    {
      return !p;
    }

    template <class O /* = T */>
    static typename object_traits<O>::id_type
    object_id (const pointer_type& p)
    {
      return p.template object_id<O> ();
    }
  };

  template <typename T>
  class pointer_traits<boost::lazy_weak_ptr<T> >
  {
  public:
    static const pointer_kind kind = pk_weak;
    static const bool lazy = true;

    typedef T element_type;
    typedef boost::lazy_weak_ptr<element_type> pointer_type;
    typedef boost::lazy_shared_ptr<element_type> strong_pointer_type;
    typedef ::boost::weak_ptr<element_type> eager_pointer_type;

    static strong_pointer_type
    lock (const pointer_type& p)
    {
      return p.lock ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_BOOST_SMART_PTR_LAZY_POINTER_TRAITS_HXX
