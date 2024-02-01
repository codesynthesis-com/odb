// file      : odb/boost/exception.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_EXCEPTION_HXX
#define ODB_BOOST_EXCEPTION_HXX

#include <odb/pre.hxx>

#include <odb/exceptions.hxx>

#include <odb/details/config.hxx> // ODB_NOTHROW_NOEXCEPT
#include <odb/boost/details/export.hxx>

namespace odb
{
  namespace boost
  {
    struct LIBODB_BOOST_EXPORT exception: odb::exception
    {
      virtual const char*
      what () const ODB_NOTHROW_NOEXCEPT = 0;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_EXCEPTION_HXX
