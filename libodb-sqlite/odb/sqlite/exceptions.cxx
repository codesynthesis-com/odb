// file      : odb/sqlite/exceptions.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sstream>

#include <odb/sqlite/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace sqlite
  {
    //
    // forced_rollback
    //

    const char* forced_rollback::
    what () const ODB_NOTHROW_NOEXCEPT
    {
      return "transaction is forced to rollback";
    }

    forced_rollback* forced_rollback::
    clone () const
    {
      return new forced_rollback (*this);
    }

    //
    // database_exception
    //

    database_exception::
    ~database_exception () ODB_NOTHROW_NOEXCEPT
    {
    }

    database_exception::
    database_exception (int e, int ee, const string& m)
        : error_ (e), extended_error_ (ee), message_ (m)
    {
      ostringstream ostr;
      ostr << error_;

      if (error_ != extended_error_)
        ostr << " (" << extended_error_ << ")";

      ostr << ": " << message_;
      what_ = ostr.str ();
    }

    const char* database_exception::
    what () const ODB_NOTHROW_NOEXCEPT
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
    cli_exception (const std::string& what)
        : what_ (what)
    {
    }

    cli_exception::
    ~cli_exception () ODB_NOTHROW_NOEXCEPT
    {
    }

    const char* cli_exception::
    what () const ODB_NOTHROW_NOEXCEPT
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
