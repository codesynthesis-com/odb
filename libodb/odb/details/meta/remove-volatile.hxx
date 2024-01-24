// file      : odb/details/meta/remove-volatile.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_META_REMOVE_VOLATILE_HXX
#define ODB_DETAILS_META_REMOVE_VOLATILE_HXX

#include <odb/pre.hxx>

namespace odb
{
  namespace details
  {
    namespace meta
    {
      template <typename X>
      struct remove_volatile
      {
        typedef X result;
      };

      template <typename X>
      struct remove_volatile<volatile X>
      {
        typedef X result;
      };
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_DETAILS_META_REMOVE_VOLATILE_HXX
