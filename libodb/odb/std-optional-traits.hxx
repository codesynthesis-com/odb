// file      : odb/std-optional-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_STD_OPTIONAL_TRAITS_HXX
#define ODB_STD_OPTIONAL_TRAITS_HXX

#include <odb/pre.hxx>

#include <optional>
#include <type_traits> // std::remove_const

#include <odb/wrapper-traits.hxx>

namespace odb
{
  template <typename T>
  class wrapper_traits<std::optional<T>>
  {
  public:
    using wrapped_type = T;
    using wrapper_type = std::optional<T>;

    // T can be const.
    //
    using unrestricted_wrapped_type = typename std::remove_const<T>::type;

    static const bool null_handler = true;
    static const bool null_default = true;

    static bool
    get_null (const wrapper_type& o)
    {
      return !o;
    }

    static void
    set_null (wrapper_type& o)
    {
      o = std::nullopt;
    }

    static const wrapped_type&
    get_ref (const wrapper_type& o)
    {
      return *o;
    }

    static unrestricted_wrapped_type&
    set_ref (wrapper_type& o)
    {
      if (!o)
        o = unrestricted_wrapped_type ();

      return const_cast<unrestricted_wrapped_type&> (*o);
    }
  };
}

#include <odb/post.hxx>

#endif // ODB_STD_OPTIONAL_TRAITS_HXX
