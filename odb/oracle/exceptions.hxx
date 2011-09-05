// file      : odb/oracle/exceptions.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_EXCEPTIONS_HXX
#define ODB_ORACLE_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <string>

#include <odb/exceptions.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    struct LIBODB_ORACLE_EXPORT database_exception: odb::database_exception
    {
      database_exception (int error, const std::string& message);

      ~database_exception () throw ();

      int
      error () const
      {
        return error_;
      }

      const std::string&
      message () const
      {
        return message_;
      }

      virtual const char*
      what () const throw ();

    private:
      int error_;
      std::string message_;
    };

    struct LIBODB_ORACLE_EXPORT cli_exception: odb::exception
    {
      cli_exception (const std::string& what);
      ~cli_exception () throw ();

      virtual const char*
      what () const throw ();

    private:
      std::string what_;
    };

    struct LIBODB_ORACLE_EXPORT invalid_oci_handle: odb::exception
    {
      virtual const char*
      what () const throw ();
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_EXCEPTIONS_HXX
