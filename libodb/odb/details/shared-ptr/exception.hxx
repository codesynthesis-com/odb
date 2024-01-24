// file      : odb/details/shared-ptr/exception.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_SHARED_PTR_EXCEPTION_HXX
#define ODB_DETAILS_SHARED_PTR_EXCEPTION_HXX

#include <odb/pre.hxx>

#include <odb/exception.hxx>

#include <odb/details/config.hxx> // ODB_NOTHROW_NOEXCEPT
#include <odb/details/export.hxx>

namespace odb
{
  namespace details
  {
    struct LIBODB_EXPORT not_shared: exception
    {
      virtual const char*
      what () const ODB_NOTHROW_NOEXCEPT;

      virtual not_shared*
      clone () const;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_DETAILS_SHARED_PTR_EXCEPTION_HXX
