// file      : odb/mssql/exceptions.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <sstream>

#include <odb/mssql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    //
    // database_exception
    //

    database_exception::record::
    record (SQLINTEGER e, const string& s, const string& m)
        : error_ (e), sqlstate_ (s), message_ (m)
    {
    }

    database_exception::
    ~database_exception () throw ()
    {
    }

    database_exception::
    database_exception ()
    {
    }

    database_exception::
    database_exception (SQLINTEGER e, const string& s, const string& m)
    {
      append (e, s, m);
    }

    void database_exception::
    append (SQLINTEGER e, const string& s, const string& m)
    {
      records_.push_back (record (e, s, m));

      if (!what_.empty ())
        what_ += '\n';

      ostringstream ostr;
      ostr << e << " (" << s << "): " << m;
      what_ += ostr.str ();
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

    //
    // long_data_reload
    //

    const char* long_data_reload::
    what () const throw ()
    {
      return "attempt to re-load object or view with long data "
        "from query result";
    }

    long_data_reload* long_data_reload::
    clone () const
    {
      return new long_data_reload (*this);
    }
  }
}
