// file      : odb/mysql/exceptions.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_EXCEPTIONS_HXX
#define ODB_MYSQL_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <string>

#include <odb/exceptions.hxx>
#include <odb/details/config.hxx> // ODB_NOTHROW_NOEXCEPT

#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    struct LIBODB_MYSQL_EXPORT database_exception: odb::database_exception
    {
      database_exception (unsigned int,
                          const std::string& sqlstate,
                          const std::string& message);

      ~database_exception () ODB_NOTHROW_NOEXCEPT;

      unsigned int
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

      virtual const char*
      what () const ODB_NOTHROW_NOEXCEPT;

      virtual database_exception*
      clone () const;

    private:
      unsigned int error_;
      std::string sqlstate_;
      std::string message_;
      std::string what_;
    };

    struct LIBODB_MYSQL_EXPORT cli_exception: odb::exception
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
      using mysql::database_exception;
      using mysql::cli_exception;
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_EXCEPTIONS_HXX
