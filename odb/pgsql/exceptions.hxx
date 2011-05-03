// file      : odb/pgsql/exceptions.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_EXCEPTIONS_HXX
#define ODB_PGSQL_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <string>

#include <odb/exceptions.hxx>

#include <odb/pgsql/version.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    struct LIBODB_PGSQL_EXPORT cli_exception: odb::exception
    {
      cli_exception (const std::string& what);
      ~cli_exception () throw ();

      virtual const char*
      what () const throw ();

    private:
      std::string what_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_EXCEPTIONS_HXX
