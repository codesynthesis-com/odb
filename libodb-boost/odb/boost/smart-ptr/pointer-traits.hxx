// file      : odb/boost/smart-ptr/pointer-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_SMART_PTR_POINTER_TRAITS_HXX
#define ODB_BOOST_SMART_PTR_POINTER_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <odb/pointer-traits.hxx>
#include <odb/details/meta/remove-const.hxx>

namespace odb
{
  // Specialization for boost::shared_ptr.
  //
  template <typename T>
  class pointer_traits< ::boost::shared_ptr<T> >
  {
  public:
    static const pointer_kind kind = pk_shared;
    static const bool lazy = false;

    typedef T element_type;
    typedef ::boost::shared_ptr<element_type> pointer_type;
    typedef ::boost::shared_ptr<const element_type> const_pointer_type;
    typedef typename odb::details::meta::remove_const<element_type>::result
    unrestricted_element_type;
    typedef ::boost::shared_ptr<unrestricted_element_type>
    unrestricted_pointer_type;
    typedef smart_ptr_guard<pointer_type> guard;

    static element_type*
    get_ptr (const pointer_type& p)
    {
      return p.get ();
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
      return ::boost::const_pointer_cast<unrestricted_element_type> (p);
    }

    template <typename T1>
    static ::boost::shared_ptr<T1>
    static_pointer_cast (const pointer_type& p)
    {
      return ::boost::static_pointer_cast<T1> (p);
    }

    template <typename T1>
    static ::boost::shared_ptr<T1>
    dynamic_pointer_cast (const pointer_type& p)
    {
      return ::boost::dynamic_pointer_cast<T1> (p);
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

  // Specialization for boost::weak_ptr.
  //
  template <typename T>
  class pointer_traits< ::boost::weak_ptr<T> >
  {
  public:
    static const pointer_kind kind = pk_weak;
    static const bool lazy = false;

    typedef T element_type;
    typedef ::boost::weak_ptr<element_type> pointer_type;
    typedef ::boost::shared_ptr<element_type> strong_pointer_type;

    static strong_pointer_type
    lock (const pointer_type& p)
    {
      return p.lock ();
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_BOOST_SMART_PTR_POINTER_TRAITS_HXX
