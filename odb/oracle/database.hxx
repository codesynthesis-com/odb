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
#include <odb/oracle/query.hxx>
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
      erase_query (const oracle::query_base&);

      template <typename T>
      unsigned long long
      erase_query (const odb::query_base&);

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
      query (const oracle::query_base&);

      template <typename T>
      result<T>
      query (const odb::query_base&);

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
      prepare_query (const char* name, const oracle::query_base&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const odb::query_base&);

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
