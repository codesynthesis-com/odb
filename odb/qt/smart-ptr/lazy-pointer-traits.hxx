// file      : odb/qt/smart-ptr/lazy-pointer-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_SMART_PTR_LAZY_POINTER_TRAITS_HXX
#define ODB_QT_SMART_PTR_LAZY_POINTER_TRAITS_HXX

#include <odb/pre.hxx>

#include <odb/pointer-traits.hxx>
#include <odb/qt/smart-ptr/lazy-ptr.hxx>

namespace odb
{
  template <typename T>
  class pointer_traits<QLazySharedPointer<T> >
  {
  public:
    static const pointer_kind kind = pk_shared;
    static const bool lazy = true;

    typedef T element_type;
    typedef QLazySharedPointer<element_type> pointer_type;
    typedef QSharedPointer<element_type> eager_pointer_type;

    static bool
    null_ptr (const pointer_type& p)
    {
      return !p;
    }

    template <class O /* = T */>
    static typename object_traits<O>::id_type
    object_id (const pointer_type& p)
    {
      return p.template objectId<O> ();
    }
  };

  template <typename T>
  class pointer_traits<QLazyWeakPointer<T> >
  {
  public:
    static const pointer_kind kind = pk_weak;
    static const bool lazy = true;

    typedef T element_type;
    typedef QLazyWeakPointer<element_type> pointer_type;
    typedef QLazySharedPointer<element_type> strong_pointer_type;
    typedef QWeakPointer<element_type> eager_pointer_type;

    static strong_pointer_type
    lock (const pointer_type& p)
    {
      return p.toStrongRef ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_QT_SMART_PTR_LAZY_POINTER_TRAITS_HXX
