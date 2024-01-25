// file      : odb/mysql/database.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_DATABASE_HXX
#define ODB_MYSQL_DATABASE_HXX

#include <odb/pre.hxx>

#include <string>
#include <iosfwd> // std::ostream

#include <odb/database.hxx>
#include <odb/details/config.hxx> // ODB_CXX11
#include <odb/details/unique-ptr.hxx>
#include <odb/details/transfer-ptr.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/query.hxx>
#include <odb/mysql/tracer.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/connection-factory.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class transaction_impl;

    class LIBODB_MYSQL_EXPORT database: public odb::database
    {
    public:
      // In MySQL, NULL and empty string are treated as the same value
      // for all the arguments except passwd and socket.
      //
      database (const char* user,
                const char* passwd,
                const char* db,
                const char* host = 0,
                unsigned int port = 0,
                const char* socket = 0,
                const char* charset = 0,
                unsigned long client_flags = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      database (const std::string& user,
                const std::string& passwd,
                const std::string& db,
                const std::string& host = "",
                unsigned int port = 0,
                const std::string* socket = 0,
                const std::string& charset = "",
                unsigned long client_flags = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      database (const std::string& user,
                const std::string* passwd,
                const std::string& db,
                const std::string& host = "",
                unsigned int port = 0,
                const std::string* socket = 0,
                const std::string& charset = "",
                unsigned long client_flags = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      database (const std::string& user,
                const std::string& passwd,
                const std::string& db,
                const std::string& host,
                unsigned int port,
                const std::string& socket,
                const std::string& charset = "",
                unsigned long client_flags = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      database (const std::string& user,
                const std::string* passwd,
                const std::string& db,
                const std::string& host,
                unsigned int port,
                const std::string& socket,
                const std::string& charset = "",
                unsigned long client_flags = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // Extract the database parameters from the command line. The
      // following options are recognized:
      //
      // --user
      // --password
      // --database
      // --host
      // --port
      // --socket
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
                const std::string& charset = "",
                unsigned long client_flags = 0,
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // Move-constructible but not move-assignable.
      //
      // Note: noexcept is not specified since odb::database(odb::database&&)
      // can throw.
      //
#ifdef ODB_CXX11
      database (database&&);
#endif

      static void
      print_usage (std::ostream&);

    public:
      const char*
      user () const
      {
        return user_.c_str ();
      }

      const char*
      password () const
      {
        return passwd_;
      }

      const char*
      db () const
      {
        return db_.c_str ();
      }

      const char*
      host () const
      {
        return host_.c_str ();
      }

      unsigned int
      port () const
      {
        return port_;
      }

      const char*
      socket () const
      {
        return socket_;
      }

      const char*
      charset () const
      {
        return charset_.c_str ();
      }

      unsigned long
      client_flags () const
      {
        return client_flags_;
      }

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
      persist (const T& object);

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

      // Load (or reload, if it is already loaded) a section of an object.
      //
      template <typename T>
      void
      load (T& object, section&);

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

      // Update a section of an object. Throws the section_not_loaded
      // exception if the section is not loaded. Note also that this
      // function does not clear the changed flag if it is set.
      //
      template <typename T>
      void
      update (const T& object, const section&);

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
      erase_query (const mysql::query_base&);

      template <typename T>
      unsigned long long
      erase_query (const odb::query_base&);

      // Query API.
      //
      template <typename T>
      result<T>
      query (bool cache = true);

      template <typename T>
      result<T>
      query (const char*, bool cache = true);

      template <typename T>
      result<T>
      query (const std::string&, bool cache = true);

      template <typename T>
      result<T>
      query (const mysql::query_base&, bool cache = true);

      template <typename T>
      result<T>
      query (const odb::query_base&, bool cache = true);

      // Query one API.
      //
      template <typename T>
      typename result<T>::pointer_type
      query_one ();

      template <typename T>
      bool
      query_one (T& object);

      template <typename T>
      T
      query_value ();

      template <typename T>
      typename result<T>::pointer_type
      query_one (const char*);

      template <typename T>
      bool
      query_one (const char*, T& object);

      template <typename T>
      T
      query_value (const char*);

      template <typename T>
      typename result<T>::pointer_type
      query_one (const std::string&);

      template <typename T>
      bool
      query_one (const std::string&, T& object);

      template <typename T>
      T
      query_value (const std::string&);

      template <typename T>
      typename result<T>::pointer_type
      query_one (const mysql::query_base&);

      template <typename T>
      bool
      query_one (const mysql::query_base&, T& object);

      template <typename T>
      T
      query_value (const mysql::query_base&);

      template <typename T>
      typename result<T>::pointer_type
      query_one (const odb::query_base&);

      template <typename T>
      bool
      query_one (const odb::query_base&, T& object);

      template <typename T>
      T
      query_value (const odb::query_base&);

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
      prepare_query (const char* name, const mysql::query_base&);

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

      // SQL statement tracing.
      //
    public:
      typedef mysql::tracer tracer_type;

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

      // Database schema version.
      //
    protected:
      virtual const schema_version_info&
      load_schema_version (const std::string& schema_name) const;

    public:
      // Database id constant (useful for meta-programming).
      //
      static const odb::database_id database_id = id_mysql;

    public:
      virtual
      ~database ();

    protected:
      virtual odb::connection*
      connection_ ();

    private:
      // Note: remember to update move ctor if adding any new members.
      //
      std::string user_;
      std::string passwd_str_;
      const char* passwd_;
      std::string db_;
      std::string host_;
      unsigned int port_;
      std::string socket_str_;
      const char* socket_;
      std::string charset_;
      unsigned long client_flags_;
      details::unique_ptr<connection_factory> factory_;
    };
  }
}

#include <odb/mysql/database.ixx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_DATABASE_HXX
