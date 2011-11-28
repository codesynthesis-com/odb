// file      : odb/mssql/exceptions.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_EXCEPTIONS_HXX
#define ODB_MSSQL_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>

#include <odb/exceptions.hxx>

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>
#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    struct LIBODB_MSSQL_EXPORT database_exception: odb::database_exception
    {
      struct record
      {
        record (SQLINTEGER error,
                const std::string& sqlstate,
                const std::string& message);

        SQLINTEGER
        error () const
        {
          return error_;
        }

        const std::string&
        sqlstate () const
        {
          return sqlstate_;
        }

        const std::string&
        message () const
        {
          return message_;
        }

      private:
        SQLINTEGER error_;
        std::string sqlstate_;
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

      virtual const char*
      what () const throw ();

    public:
      ~database_exception () throw ();

      database_exception ();
      database_exception (SQLINTEGER error,
                          const std::string& sqlstate,
                          const std::string& message);

      void
      append (SQLINTEGER error,
              const std::string& sqlstate,
              const std::string& message);

    private:
      records records_;
      std::string what_;
    };

    struct LIBODB_MSSQL_EXPORT cli_exception: odb::exception
    {
      cli_exception (const std::string& what);
      ~cli_exception () throw ();

      virtual const char*
      what () const throw ();

    private:
      std::string what_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_EXCEPTIONS_HXX
