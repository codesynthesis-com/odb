// file      : odb/mysql/exceptions.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sstream>

#include <odb/mysql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
    //
    // database_exception
    //

    database_exception::
    ~database_exception () noexcept
    {
    }

    database_exception::
    database_exception (unsigned int e, const string& s, const string& m)
        : error_ (e), sqlstate_ (s), message_ (m)
    {
      ostringstream ostr;
      ostr << error_ << " (" << sqlstate_ << "): " << message_;
      what_ = ostr.str ();
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
    cli_exception (const std::string& what)
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
