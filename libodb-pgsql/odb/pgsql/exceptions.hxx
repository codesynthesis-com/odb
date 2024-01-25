// file      : odb/pgsql/exceptions.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_EXCEPTIONS_HXX
#define ODB_PGSQL_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <string>

#include <odb/exceptions.hxx>
#include <odb/details/config.hxx> // ODB_NOTHROW_NOEXCEPT

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    struct LIBODB_PGSQL_EXPORT database_exception: odb::database_exception
    {
      database_exception (const std::string& message);

      database_exception (const std::string& sqlstate,
                          const std::string& message);

      ~database_exception () ODB_NOTHROW_NOEXCEPT;

      const std::string&
      message () const
      {
        return message_;
      }

      const std::string&
      sqlstate () const
      {
        return sqlstate_;
      }

      virtual const char*
      what () const ODB_NOTHROW_NOEXCEPT;

      virtual database_exception*
      clone () const;

    private:
      std::string sqlstate_;
      std::string message_;
      std::string what_;
    };

    struct LIBODB_PGSQL_EXPORT cli_exception: odb::exception
    {
      cli_exception (const std::string& what);
      ~cli_exception () ODB_NOTHROW_NOEXCEPT;

      virtual const char*
      what () const ODB_NOTHROW_NOEXCEPT;

      virtual cli_exception*
      clone () const;

    private:
      std::string what_;
    };

    namespace core
    {
      using pgsql::database_exception;
      using pgsql::cli_exception;
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_EXCEPTIONS_HXX
