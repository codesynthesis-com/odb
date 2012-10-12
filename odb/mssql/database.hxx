// file      : odb/mssql/database.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_DATABASE_HXX
#define ODB_MSSQL_DATABASE_HXX

#include <odb/pre.hxx>

#include <string>
#include <memory> // std::auto_ptr, std::unique_ptr
#include <iosfwd> // std::ostream

#include <odb/database.hxx>
#include <odb/details/unique-ptr.hxx>
#include <odb/details/transfer-ptr.hxx>

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/query.hxx>
#include <odb/mssql/tracer.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/connection-factory.hxx>
#include <odb/mssql/auto-handle.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class transaction_impl;

    enum protocol
    {
      protocol_auto,
      protocol_tcp, // TCP/IP.
      protocol_lpc, // Shared memory (local procedure call).
      protocol_np   // Named pipes.
    };

    class LIBODB_MSSQL_EXPORT database: public odb::database
    {
    public:
      typedef mssql::protocol protocol_type;

      // Connect to the specified server using the latest available SQL
      // Server Native Client ODBC driver by default. If user is empty,
      // then use Windows authentication. If db is empty, then use the
      // default database for this user.
      //
      database (const std::string& user,
                const std::string& password,
                const std::string& db,
                const std::string& server,
                const std::string& driver = "",
                const std::string& extra_connect_string = "",
                SQLHENV environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // By default connect to the default instance on localhost using
      // default protocol and the latest available SQL Server Native
      // Client ODBC driver. If user is empty, then use Windows
      // authentication. If db is empty, then use the default database
      // for this user.
      //
      database (const std::string& user,
                const std::string& password,
                const std::string& db,
                protocol_type protocol = protocol_auto,
                const std::string& host = "",
                const std::string& instance = "",
                const std::string& driver = "",
                const std::string& extra_connect_string = "",
                SQLHENV environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // Connect using TCP/IP to the specified host and port. If port is
      // 0, use the default port (1433).
      //
      database (const std::string& user,
                const std::string& password,
                const std::string& db,
                const std::string& host,
                unsigned int port,
                const std::string& driver = "",
                const std::string& extra_connect_string = "",
                SQLHENV environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // Connect using a custom SQL Server Native Client ODBC driver
      // conection string.
      //
      database (const std::string& connect_string,
                SQLHENV environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // Extract the database parameters from the command line. The
      // following options are recognized:
      //
      // --user     | -U
      // --password | -P
      // --database | -d
      // --server   | -S
      // --driver
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
                const std::string& extra_connect_string = "",
                SQLHENV environment = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      static void
      print_usage (std::ostream&);

      // Object persistence API.
      //
    public:

      // Make the object persistent.
      //
      template <typename T>
      typename object_traits<T>::id_type
      persist (T& object);

      template <typename T>
      typename object_traits<T>::id_type
      persist (T* obj_ptr);

      template <typename T, template <typename> class P>
      typename object_traits<T>::id_type
      persist (const P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      typename object_traits<T>::id_type
      persist (const P<T, A1>& obj_ptr);

      template <typename T, template <typename> class P>
      typename object_traits<T>::id_type
      persist (P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      typename object_traits<T>::id_type
      persist (P<T, A1>& obj_ptr);

      template <typename T>
      typename object_traits<T>::id_type
      persist (const typename object_traits<T>::pointer_type& obj_ptr);

      // Load an object. Throw object_not_persistent if not found.
      //
      template <typename T>
      typename object_traits<T>::pointer_type
      load (const typename object_traits<T>::id_type& id);

      template <typename T>
      void
      load (const typename object_traits<T>::id_type& id, T& object);

      // Reload an object.
      //
      template <typename T>
      void
      reload (T& object);

      template <typename T>
      void
      reload (T* obj_ptr);

      template <typename T, template <typename> class P>
      void
      reload (const P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      void
      reload (const P<T, A1>& obj_ptr);

      template <typename T, template <typename> class P>
      void
      reload (P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      void
      reload (P<T, A1>& obj_ptr);

      template <typename T>
      void
      reload (const typename object_traits<T>::pointer_type& obj_ptr);

      // Loan an object if found. Return NULL/false if not found.
      //
      template <typename T>
      typename object_traits<T>::pointer_type
      find (const typename object_traits<T>::id_type& id);

      template <typename T>
      bool
      find (const typename object_traits<T>::id_type& id, T& object);

      // Update the state of a modified objects.
      //
      template <typename T>
      void
      update (T& object);

      template <typename T>
      void
      update (T* obj_ptr);

      template <typename T, template <typename> class P>
      void
      update (const P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      void
      update (const P<T, A1>& obj_ptr);

      template <typename T, template <typename> class P>
      void
      update (P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      void
      update (P<T, A1>& obj_ptr);

      template <typename T>
      void
      update (const typename object_traits<T>::pointer_type& obj_ptr);

      // Make the object transient. Throw object_not_persistent if not
      // found.
      //
      template <typename T>
      void
      erase (const typename object_traits<T>::id_type& id);

      template <typename T>
      void
      erase (T& object);

      template <typename T>
      void
      erase (T* obj_ptr);

      template <typename T, template <typename> class P>
      void
      erase (const P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      void
      erase (const P<T, A1>& obj_ptr);

      template <typename T, template <typename> class P>
      void
      erase (P<T>& obj_ptr);

      template <typename T, typename A1, template <typename, typename> class P>
      void
      erase (P<T, A1>& obj_ptr);

      template <typename T>
      void
      erase (const typename object_traits<T>::pointer_type& obj_ptr);

      // Erase multiple objects matching a query predicate.
      //
      template <typename T>
      unsigned long long
      erase_query ();

      template <typename T>
      unsigned long long
      erase_query (const char*);

      template <typename T>
      unsigned long long
      erase_query (const std::string&);

      template <typename T>
      unsigned long long
      erase_query (const mssql::query<T>&);

      // Query API.
      //
      template <typename T>
      result<T>
      query ();

      template <typename T>
      result<T>
      query (const char*);

      template <typename T>
      result<T>
      query (const std::string&);

      template <typename T>
      result<T>
      query (const mssql::query<T>&);

      // Query preparation.
      //
      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const char*);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const std::string&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const mssql::query<T>&);

      // Transactions.
      //
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

      protocol_type
      protocol () const
      {
        return protocol_;
      }

      const std::string&
      host () const
      {
        return host_;
      }

      const std::string&
      instance () const
      {
        return instance_;
      }

      unsigned int
      port () const
      {
        return port_;
      }

      const std::string&
      server () const
      {
        return server_;
      }

      const std::string&
      driver () const
      {
        return driver_;
      }

      const std::string&
      extra_connect_string () const
      {
        return extra_connect_string_;
      }

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

    public:
      virtual
      ~database ();

    protected:
      virtual odb::connection*
      connection_ ();

    private:
      void
      init ();

    private:
      std::string user_;
      std::string password_;
      std::string db_;
      protocol_type protocol_;
      std::string host_;
      std::string instance_;
      unsigned int port_;
      std::string server_;
      std::string driver_;
      std::string extra_connect_string_;
      std::string connect_string_;

      auto_handle<SQL_HANDLE_ENV> auto_environment_;
      SQLHENV environment_;

      details::unique_ptr<connection_factory> factory_;
    };
  }
}

#include <odb/mssql/database.ixx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_DATABASE_HXX
