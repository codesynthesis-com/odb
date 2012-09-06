// file      : odb/oracle/database.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_DATABASE_HXX
#define ODB_ORACLE_DATABASE_HXX

#include <odb/pre.hxx>

#include <string>
#include <memory> // std::auto_ptr, std::unique_ptr
#include <iosfwd> // std::ostream

#include <odb/database.hxx>
#include <odb/details/unique-ptr.hxx>
#include <odb/details/transfer-ptr.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/tracer.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/connection-factory.hxx>
#include <odb/oracle/auto-handle.hxx>
#include <odb/oracle/oracle-fwd.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class transaction_impl;

    class LIBODB_ORACLE_EXPORT database: public odb::database
    {
    public:
      database (const std::string& user,
                const std::string& password,
                const std::string& db,
                ub2 charset = 0,
                ub2 ncharset = 0,
                OCIEnv* environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      database (const std::string& user,
                const std::string& password,
                const std::string& service,
                const std::string& host,
                unsigned int port = 0,
                ub2 charset = 0,
                ub2 ncharset = 0,
                OCIEnv* environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

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
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      static void
      print_usage (std::ostream&);

    public:
      virtual transaction_impl*
      begin ();

    public:
      connection_ptr
      connection ();

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

      ub2
      charset () const
      {
        return charset_;
      }

      ub2
      ncharset () const
      {
        return ncharset_;
      }

      OCIEnv*
      environment ()
      {
        return environment_;
      }

      // SQL statement tracing.
      //
    public:
      typedef oracle::tracer tracer_type;

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

    public:
      virtual
      ~database ();

    protected:
      virtual odb::connection*
      connection_ ();

    private:
      std::string user_;
      std::string password_;

      std::string db_;

      std::string service_;
      std::string host_;
      unsigned int port_;

      ub2 charset_;
      ub2 ncharset_;

      auto_handle<OCIEnv> auto_environment_;
      OCIEnv* environment_;

      details::unique_ptr<connection_factory> factory_;
    };
  }
}

#include <odb/oracle/database.ixx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_DATABASE_HXX
