// file      : odb/boost/smart-ptr/wrapper-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_SMART_PTR_WRAPPER_TRAITS_HXX
#define ODB_BOOST_SMART_PTR_WRAPPER_TRAITS_HXX

#include <odb/pre.hxx>

#include <boost/shared_ptr.hpp>

#include <odb/wrapper-traits.hxx>

namespace odb
{
  // Specialization for boost::shared_ptr.
  //
  template <typename T>
  class wrapper_traits< ::boost::shared_ptr<T> >
  {
  public:
    typedef T wrapped_type;
    typedef ::boost::shared_ptr<T> wrapper_type;

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
      return !p;
    }

    static void
    set_null (wrapper_type& p)
    {
      p.reset ();
    }

    static const wrapped_type&
    get_ref (const wrapper_type& p)
    {
      return *p;
    }

    static unrestricted_wrapped_type&
    set_ref (wrapper_type& p)
    {
      if (!p)
        p.reset (new unrestricted_wrapped_type);

      return const_cast<unrestricted_wrapped_type&> (*p);
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_BOOST_SMART_PTR_WRAPPER_TRAITS_HXX
