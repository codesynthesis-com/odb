// file      : odb/details/win32/exceptions.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_WIN32_EXCEPTIONS_HXX
#define ODB_DETAILS_WIN32_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <odb/details/config.hxx> // ODB_NOTHROW_NOEXCEPT
#include <odb/details/export.hxx>
#include <odb/details/exception.hxx>
#include <odb/details/win32/windows.hxx>

namespace odb
{
  namespace details
  {
    struct LIBODB_EXPORT win32_exception: details::exception
    {
      win32_exception () : code_ (GetLastError ()) {}
      win32_exception (DWORD code) : code_ (code) {}

      DWORD
      code () const {return code_;}

      virtual const char*
      what () const ODB_NOTHROW_NOEXCEPT;

      virtual win32_exception*
      clone () const;

    private:
      DWORD code_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_DETAILS_WIN32_EXCEPTIONS_HXX
