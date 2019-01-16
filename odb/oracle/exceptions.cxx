// file      : odb/oracle/exceptions.cxx
// copyright : Copyright (c) 2005-2019 Code Synthesis Tools CC
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
    ~database_exception () ODB_NOTHROW_NOEXCEPT
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
    // lob_comparison
    //

    const char* lob_comparison::
    what () const ODB_NOTHROW_NOEXCEPT
    {
      return "comparison of LOB values in queries not supported";
    }

    lob_comparison* lob_comparison::
    clone () const
    {
      return new lob_comparison (*this);
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

    //
    // invalid_oci_handle
    //

    const char* invalid_oci_handle::
    what () const ODB_NOTHROW_NOEXCEPT
    {
      return "invalid oci handle passed or unable to allocate handle";
    }

    invalid_oci_handle* invalid_oci_handle::
    clone () const
    {
      return new invalid_oci_handle (*this);
    }
  }
}
