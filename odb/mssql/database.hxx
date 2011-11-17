// file      : odb/mssql/database.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_DATABASE_HXX
#define ODB_MSSQL_DATABASE_HXX

//@@ disabled functionality

#include <odb/pre.hxx>

#include <string>
#include <memory> // std::auto_ptr
#include <iosfwd> // std::ostream

#include <odb/database.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
//#include <odb/mssql/tracer.hxx>
//#include <odb/mssql/connection.hxx>
//#include <odb/mssql/connection-factory.hxx>
#include <odb/mssql/auto-handle.hxx>
#include <odb/mssql/mssql-fwd.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class transaction_impl;

    class LIBODB_MSSQL_EXPORT database: public odb::database
    {
    public:
      database (const std::string& connect_string,
                SQLHENV environment = 0,
                std::auto_ptr<connection_factory> factory =
                  std::auto_ptr<connection_factory> (0));

      /*
      database (const std::string& user,
                const std::string& password,
                const std::string& service,
                const std::string& host = "",
                unsigned int port = 0,
                ub2 charset = 0,
                ub2 ncharset = 0,
                OCIEnv* environment = 0,
                std::auto_ptr<connection_factory> factory =
                  std::auto_ptr<connection_factory> (0));

      // Extract the database parameters from the command line. The
      // following options are recognized:
      //
      // --user
      // --password
      // --database
      // --service
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
                ub2 charset = 0,
                ub2 ncharset = 0,
                OCIEnv* environment = 0,
                std::auto_ptr<connection_factory> =
                  std::auto_ptr<connection_factory> (0));

      */

      static void
      print_usage (std::ostream&);


    public:
      //virtual transaction_impl*
      //begin ();

    public:
      //connection_ptr
      //connection ();

    public:
      /*
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
      service () const
      {
        return service_;
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
      */

      const std::string&
      connect_string () const
      {
        return connect_string_;
      }

      SQLHENV
      environment ()
      {
        return environment_;
      }

      // SQL statement tracing.
      //
    public:
      /*
      typedef mssql::tracer tracer_type;

      void
      tracer (tracer_type& t)
      {
        odb::database::tracer (t);
      }

      void
      tracer (tracer_type* t)
      {
        odb::database::tracer (t);
      }

      using odb::database::tracer;
      */

    public:
      virtual
      ~database ();

    protected:
      //virtual odb::connection*
      //connection_ ();

    private:
      /*
      std::string user_;
      std::string password_;

      std::string db_;

      std::string service_;
      std::string host_;
      unsigned int port_;

      ub2 charset_;
      ub2 ncharset_;
      */

      std::string connect_string_;

      auto_handle<SQL_HANDLE_ENV> auto_environment_;
      SQLHENV environment_;

      std::auto_ptr<connection_factory> factory_;
    };
  }
}

#include <odb/mssql/database.ixx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_DATABASE_HXX
