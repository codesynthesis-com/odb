// file      : odb/pgsql/exceptions.cxx
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
      if (!sqlstate_.empty ())
        what_ = sqlstate_ + ": " + message_;
      else
        what_ = message_;
    }

    database_exception::
    ~database_exception () noexcept
    {
    }

    const char* database_exception::
    what () const noexcept
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
    ~cli_exception () noexcept
    {
    }

    const char* cli_exception::
    what () const noexcept
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
