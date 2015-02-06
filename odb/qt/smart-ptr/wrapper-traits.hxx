// file      : odb/qt/smart-ptr/wrapper-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_SMART_PTR_WRAPPER_TRAITS_HXX
#define ODB_QT_SMART_PTR_WRAPPER_TRAITS_HXX

#include <odb/pre.hxx>

#include <QtCore/QSharedPointer>

#include <odb/wrapper-traits.hxx>

namespace odb
{
  // Specialization for QSharedPointer.
  //
  template <typename T>
  class wrapper_traits< QSharedPointer<T> >
  {
  public:
    typedef T wrapped_type;
    typedef QSharedPointer<T> wrapper_type;

    // T can be const.
    //
    typedef
    typename odb::details::meta::remove_const<T>::result
    unrestricted_wrapped_type;

    static const bool null_handler = true;
    static const bool null_default = false;

    static bool
    get_null (const wrapper_type& p)
    {
      return p.isNull ();
    }

    static void
    set_null (wrapper_type& p)
    {
      p.clear ();
    }

    static const wrapped_type&
    get_ref (const wrapper_type& p)
    {
      return *p;
    }

    static wrapped_type&
    set_ref (wrapper_type& p)
    {
      if (p.isNull ())
        p = wrapper_type (new unrestricted_wrapped_type);

      return const_cast<unrestricted_wrapped_type&> (*p);
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_QT_SMART_PTR_WRAPPER_TRAITS_HXX
