// file      : odb/oracle/exceptions.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/oracle/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    //
    // database_exception
    //

    database_exception::
    database_exception (int error, const string& message)
        : error_ (error), message_ (message)
    {
    }

    database_exception::
    ~database_exception () throw ()
    {
    }

    const char* database_exception::
    what () const throw ()
    {
      return message_.c_str ();
    }

    //
    // cli_exception
    //

    cli_exception::
    cli_exception (const string& what)
        : what_ (what)
    {
    }

    cli_exception::
    ~cli_exception () throw ()
    {
    }

    const char* cli_exception::
    what () const throw ()
    {
      return what_.c_str ();
    }

    //
    // invalid_oci_handle
    //

    const char* invalid_oci_handle::
    what () const throw ()
    {
      return "invalid oci handle";
    }
  }
}
