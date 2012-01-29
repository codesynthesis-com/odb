// file      : odb/oracle/exceptions.cxx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <sstream>

#include <odb/oracle/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    //
    // database_exception
    //

    database_exception::record::
    record (sb4 e, const string& m)
        : error_ (e), message_ (m)
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
    database_exception (sb4 e, const string& m)
    {
      append (e, m);
    }

    void database_exception::
    append (sb4 e, const string& m)
    {
      records_.push_back (record (e, m));

      if (!what_.empty ())
        what_ += '\n';

      ostringstream ostr;
      ostr << e << ": " << m;
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

    //
    // invalid_oci_handle
    //

    const char* invalid_oci_handle::
    what () const throw ()
    {
      return "invalid oci handle passed or unable to allocate handle";
    }
  }
}
