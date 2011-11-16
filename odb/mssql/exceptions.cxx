// file      : odb/mssql/exceptions.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
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
  }
}
