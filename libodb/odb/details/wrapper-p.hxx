// file      : odb/details/wrapper-p.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_WRAPPER_P_HXX
#define ODB_DETAILS_WRAPPER_P_HXX

#include <odb/pre.hxx>

#include <type_traits> // std::false/true_type

#include <odb/wrapper-traits.hxx>

namespace odb
{
  namespace details
  {
    template <typename T>
    struct wrapper_p
    {
      template <typename M>
      static std::true_type test (typename wrapper_traits<M>::wrapped_type*);

      template <typename M>
      static std::false_type test (...);

      static constexpr bool value = decltype (test<T> (nullptr))::value;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_DETAILS_WRAPPER_P_HXX
