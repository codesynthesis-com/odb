// file      : odb/mysql/details/conversion.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_DETAILS_CONVERSION_HXX
#define ODB_MYSQL_DETAILS_CONVERSION_HXX

#include <odb/mysql/traits.hxx>

#include <odb/details/meta/answer.hxx>

namespace odb
{
  // @@ Revise this.
  //
  namespace details {}

  namespace mysql
  {
    namespace details
    {
      using namespace odb::details;

      // Detect whether conversion is specified in type_traits.
      //
      template <typename T>
      meta::yes
      conversion_p_test (typename type_traits<T>::conversion*);

      template <typename T>
      meta::no
      conversion_p_test (...);

      template <typename T>
      struct conversion_p
      {
        static const bool value =
          sizeof (conversion_p_test<T> (0)) == sizeof (meta::yes);
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

#endif // ODB_MYSQL_DETAILS_CONVERSION_HXX
