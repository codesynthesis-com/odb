// file      : odb/pgsql/exceptions.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sstream>

#include <odb/pgsql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    //
    // database_exception
    //

    database_exception::
    database_exception (const string& message)
        : message_ (message), what_ (message)
    {
    }

    database_exception::
    database_exception (const string& sqlstate,
                        const string& message)
        : sqlstate_ (sqlstate), message_ (message)
    {
      what_ = sqlstate_ + ": " + message_;
    }

    database_exception::
    ~database_exception () throw ()
    {
    }

    const char* database_exception::
    what () const throw ()
    {
      return what_.c_str ();
    }

    database_exception* database_exception::
    clone () const
    {
      return new database_exception (*this);
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

    cli_exception* cli_exception::
    clone () const
    {
      return new cli_exception (*this);
    }
  }
}
