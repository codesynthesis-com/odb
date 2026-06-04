// file      : odb/pgsql/details/conversion.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DETAILS_CONVERSION_HXX
#define ODB_PGSQL_DETAILS_CONVERSION_HXX

#include <odb/pgsql/traits.hxx>

#include <type_traits> // std::false/true_type

namespace odb
{
  // @@ Revise this.
  //
  namespace details {}

  namespace pgsql
  {
    namespace details
    {
      using namespace odb::details;

      // Detect whether conversion is specified in type_traits.
      //
      template <typename T>
      struct conversion_p
      {
        template <typename M>
        static std::true_type test (typename type_traits<M>::conversion*);

        template <typename M>
        static std::false_type test (...);

        static constexpr bool value = decltype (test<T> (nullptr))::value;
      };

      template <typename T, bool = conversion_p<T>::value>
      struct conversion;

      template <typename T>
      struct conversion<T, true>
      {
        static const char* to () {return type_traits<T>::conversion::to ();}
      };

      template <typename T>
      struct conversion<T, false>
      {
        static const char* to () {return 0;}
      };
    }
  }
}

#endif // ODB_PGSQL_DETAILS_CONVERSION_HXX
