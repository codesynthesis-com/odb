// file      : odb/oracle/exceptions.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_EXCEPTIONS_HXX
#define ODB_ORACLE_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>

#include <odb/exceptions.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/oracle-fwd.hxx>
#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    struct LIBODB_ORACLE_EXPORT database_exception: odb::database_exception
    {
      struct record
      {
        record (sb4 error, const std::string& message);

        sb4
        error () const
        {
          return error_;
        }

        const std::string&
        message () const
        {
          return message_;
        }

      private:
        sb4 error_;
        std::string message_;
      };

      typedef std::vector<record> records;

      typedef records::size_type size_type;
      typedef records::const_iterator iterator;

      iterator
      begin () const
      {
        return records_.begin ();
      }

      iterator
      end () const
      {
        return records_.end ();
      }

      size_type
      size () const
      {
        return records_.size ();
      }

    public:
      ~database_exception () throw ();

      database_exception ();
      database_exception (sb4 error, const std::string& message);

      virtual const char*
      what () const throw ();

      virtual database_exception*
      clone () const;

      void
      append (sb4 error, const std::string& message);

    private:
      records records_;
      std::string what_;
    };

    struct LIBODB_ORACLE_EXPORT lob_comparison: odb::exception
    {
      virtual const char*
      what () const throw ();

      virtual lob_comparison*
      clone () const;
    };

    struct LIBODB_ORACLE_EXPORT cli_exception: odb::exception
    {
      cli_exception (const std::string& what);
      ~cli_exception () throw ();

      virtual const char*
      what () const throw ();

      virtual cli_exception*
      clone () const;

    private:
      std::string what_;
    };

    struct LIBODB_ORACLE_EXPORT invalid_oci_handle: odb::exception
    {
      virtual const char*
      what () const throw ();

      virtual invalid_oci_handle*
      clone () const;
    };

    namespace core
    {
      using oracle::database_exception;
      using oracle::lob_comparison;
      using oracle::cli_exception;
      using oracle::invalid_oci_handle;
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_EXCEPTIONS_HXX
