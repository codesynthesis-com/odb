// file      : odb/qt/smart-ptr/pointer-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_SMART_PTR_POINTER_TRAITS_HXX
#define ODB_QT_SMART_PTR_POINTER_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>

#include <odb/pointer-traits.hxx>
#include <odb/details/meta/remove-const.hxx>

namespace odb
{
  // Specialization for QSharedPointer.
  //
  template <typename T>
  class pointer_traits<QSharedPointer<T> >
  {
  public:
    static const pointer_kind kind = pk_shared;
    static const bool lazy = false;

    typedef T element_type;
    typedef QSharedPointer<element_type> pointer_type;
    typedef QSharedPointer<const element_type> const_pointer_type;
    typedef typename odb::details::meta::remove_const<element_type>::result
    unrestricted_element_type;
    typedef QSharedPointer<unrestricted_element_type>
    unrestricted_pointer_type;
    typedef smart_ptr_guard<pointer_type> guard;

    static element_type*
    get_ptr (const pointer_type& p)
    {
      return p.data ();
    }

    static element_type&
    get_ref (const pointer_type& p)
    {
      return *p;
    }

    static bool
    null_ptr (const pointer_type& p)
    {
      return !p;
    }

    static unrestricted_pointer_type
    const_pointer_cast (const pointer_type& p)
    {
      return qSharedPointerConstCast<unrestricted_element_type> (p);
    }

    template <typename T1>
    static QSharedPointer<T1>
    static_pointer_cast (const pointer_type& p)
    {
      return qSharedPointerCast<T1> (p);
    }

    template <typename T1>
    static QSharedPointer<T1>
    dynamic_pointer_cast (const pointer_type& p)
    {
      return qSharedPointerDynamicCast<T1> (p);
    }

  public:
    static void*
    allocate (std::size_t n)
    {
      return operator new (n);
    }

    static void
    free (void* p)
    {
      operator delete (p);
    }
  };

  // Specialization for QWeakPointer.
  //
  template <typename T>
  class pointer_traits<QWeakPointer<T> >
  {
  public:
    static const pointer_kind kind = pk_weak;
    static const bool lazy = false;

    typedef T element_type;
    typedef QWeakPointer<element_type> pointer_type;
    typedef QSharedPointer<element_type> strong_pointer_type;

    static strong_pointer_type
    lock (const pointer_type& p)
    {
      return p.toStrongRef ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_QT_SMART_PTR_POINTER_TRAITS_HXX
