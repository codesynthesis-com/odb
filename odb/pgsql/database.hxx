// file      : odb/pgsql/database.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DATABASE_HXX
#define ODB_PGSQL_DATABASE_HXX

#include <odb/pre.hxx>

#include <string>
#include <memory> // std::auto_ptr
#include <iosfwd> // std::ostream

#include <odb/database.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/connection-factory.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class transaction_impl;

    class LIBODB_PGSQL_EXPORT database: public odb::database
    {
    public:
      database (const std::string& user,
                const std::string& password,
                const std::string& db,
                const std::string& host = "",
                unsigned int port = 0,
                const std::string& extra_conninfo = "",
                std::auto_ptr<connection_factory> factory =
                  std::auto_ptr<connection_factory> (0));

      database (const std::string& user,
                const std::string& password,
                const std::string& db,
                const std::string& host = "",
                const std::string& socket_ext = "",
                const std::string& extra_conninfo = "",
                std::auto_ptr<connection_factory> factory =
                  std::auto_ptr<connection_factory> (0));

      explicit
      database (const std::string& conninfo,
                std::auto_ptr<connection_factory> =
                  std::auto_ptr<connection_factory> (0));

      // Extract the database parameters from the command line. The
      // following options are recognized:
      //
      // --user | --username
      // --password
      // --database | --dbname
      // --host
      // --port
      // --options-file
      //
      // For more information, see the output of the print_usage() function
      // below. If erase is true, the above options are removed from the
      // argv array and the argc count is updated accordingly. This
      // constructor may throw the cli_exception exception.
      //
      database (int& argc,
                char* argv[],
                bool erase = false,
                const std::string& extra_conninfo = "",
                std::auto_ptr<connection_factory> =
                  std::auto_ptr<connection_factory> (0));

      static void
      print_usage (std::ostream&);

      // Transactions.
      //
    public:
      virtual transaction_impl*
      begin ();

    public:
      connection_ptr
      connection ();

    public:
      virtual
      ~database ();

    protected:
      virtual odb::connection*
      connection_ ();

    public:
      const std::string&
      user () const
      {
        return user_;
      }

      const std::string&
      password () const
      {
        return password_;
      }

      const std::string&
      db () const
      {
        return db_;
      }

      const std::string&
      host () const
      {
        return host_;
      }

      unsigned int
      port () const
      {
        return port_;
      }

      const std::string&
      socket_ext () const
      {
        return socket_ext_;
      }

      const std::string&
      extra_conninfo () const
      {
        return extra_conninfo_;
      }

      const std::string&
      conninfo () const
      {
        return conninfo_;
      }

    private:
      std::string user_;
      std::string password_;
      std::string db_;
      std::string host_;
      unsigned int port_;
      std::string socket_ext_;
      std::string extra_conninfo_;
      std::string conninfo_;
      std::auto_ptr<connection_factory> factory_;
    };
  }
}

#include <odb/pgsql/database.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_DATABASE_HXX
