// file      : odb/exception.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_EXCEPTION_HXX
#define ODB_EXCEPTION_HXX

#include <odb/pre.hxx>

#include <exception>

#include <odb/forward.hxx>        // odb::core

#include <odb/details/config.hxx> // ODB_NOTHROW_NOEXCEPT
#include <odb/details/export.hxx>
#include <odb/details/shared-ptr/base.hxx>

namespace odb
{
  struct LIBODB_EXPORT exception: std::exception, details::shared_base
  {
    virtual const char*
    what () const ODB_NOTHROW_NOEXCEPT = 0;

    virtual exception*
    clone () const = 0;
  };

  namespace common
  {
    using odb::exception;
  }
}

#include <odb/post.hxx>

#endif // ODB_EXCEPTION_HXX
