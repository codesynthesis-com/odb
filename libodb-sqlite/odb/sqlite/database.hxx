// file      : odb/sqlite/database.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_DATABASE_HXX
#define ODB_SQLITE_DATABASE_HXX

#include <odb/pre.hxx>

#include <sqlite3.h>

#include <string>
#include <iosfwd> // std::ostream

#include <odb/database.hxx>
#include <odb/details/config.hxx> // ODB_CXX11
#include <odb/details/unique-ptr.hxx>
#include <odb/details/transfer-ptr.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/query.hxx>
#include <odb/sqlite/tracer.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/connection-factory.hxx>
#include <odb/sqlite/transaction-impl.hxx>

#include <odb/sqlite/details/export.hxx>

// We use the sqlite3_open_v2() flags in our interface. Define them
// for SQLite earlier that 3.5.0.
//
#if SQLITE_VERSION_NUMBER < 3005000
#  define SQLITE_OPEN_READONLY         0x00000001
#  define SQLITE_OPEN_READWRITE        0x00000002
#  define SQLITE_OPEN_CREATE           0x00000004
#endif

namespace odb
{
  namespace sqlite
  {
    class transaction_impl;

    class LIBODB_SQLITE_EXPORT database: public odb::database
    {
    public:
      database (const std::string& name,
                int flags = SQLITE_OPEN_READWRITE,
                bool foreign_keys = true,
                const std::string& vfs = "",
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

#ifdef _WIN32
      database (const std::wstring& name,
                int flags = SQLITE_OPEN_READWRITE,
                bool foreign_keys = true,
                const std::string& vfs = "",
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());
#endif

      // Extract the database parameters from the command line. The
      // following options are recognized:
      //
      // --database
      // --create
      // --read-only
      // --options-file
      //
      // For more information, see the output of the print_usage() function
      // below. If erase is true, the above options are removed from the argv
      // array and the argc count is updated accordingly. The command line
      // options override the flags passed as an argument. This constructor
      // may throw the cli_exception exception.
      //
      database (int& argc,
                char* argv[],
                bool erase = false,
                int flags = SQLITE_OPEN_READWRITE,
                bool foreign_keys = true,
                const std::string& vfs = "",
                details::transfer_ptr<connection_factory> =
                  details::transfer_ptr<connection_factory> ());

      // Attach to the specified connection a database with the specified name
      // as the specified schema. Good understanding of SQLite ATTACH/DETACH
      // DATABASE statements semantics and ODB connection management is
      // strongly recommended when using this mechanism.
      //
      // The resulting database instance is referred to as an "attached
      // database" and the connection it returns as an "attached connection"
      // (which is just a proxy for the main connection). Database operations
      // executed on the attached database or attached connection are
      // automatically translated to refer to the specified schema rather than
      // "main". For uniformity attached databases can also be created for the
      // pre-attached "main" and "temp" schemas (in this case name can be
      // anything).
      //
      // The automatic translation of the statements relies on their text
      // having references to top-level database entities (tables, indexes,
      // etc) qualified with the "main" schema. To achieve this, compile your
      // headers with `--schema main` and, if using schema migration, with
      // `--schema-version-table main.schema_version`. You must also not use
      // "main" as an object/table alias in views of native statements. For
      // optimal translation performance use 4-character schema names.
      //
      // The main connection and attached to it databases and connections are
      // all meant to be used within the same thread. In particular, the
      // attached database holds a counted reference to the main connection
      // which means the connection will not be released until all the
      // attached to this connection databases are destroyed.
      //
      // Note that in this model the attached databases are attached to the
      // main connection, not to the (main) database, which mimics the
      // underlying semantics of SQLite. An alternative model would have been
      // to notionally attach the databases to the main database and under the
      // hood automatically attach them to each returned connection. While
      // this may seem like a more convenient model in some cases, it is also
      // less flexible: the current model allows attaching a different set of
      // databases to different connections, attaching them on demand as the
      // transaction progresses, etc. Also, the more convenient model can be
      // implemented on top of this model by deriving an application-specific
      // database class and/or providing custom connection factories.
      //
      // Note that unless the name is a URI with appropriate mode, it is
      // opened with the SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE flags. So if
      // you want just SQLITE_OPEN_READWRITE, then you will need to verify its
      // existence manually prior to calling this constructor.
      //
      // Note that attaching/detaching databases withing a transaction is only
      // supported since SQLite 3.21.0.
      //
      database (const connection_ptr&,
                const std::string& name,
                const std::string& schema,
                details::transfer_ptr<attached_connection_factory> =
                  details::transfer_ptr<attached_connection_factory> ());

      // The database is automatically detached on destruction but a failure
      // to detach is ignored. To detect such a failure perform explicit
      // detach. For uniformity detaching a main database is a no-op.
      //
      void
      detach ();

      // Return the main database of an attached database. If this database
      // is main, return itself.
      //
      database&
      main_database ();

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
      const std::string&
      name () const
      {
        return name_;
      }

      // Schema name under which this database was attached or empty for the
      // main database.
      //
      const std::string&
      schema () const
      {
        return schema_;
      }

      int
      flags () const
      {
        return flags_;
      }

      bool
      foreign_keys () const
      {
        return foreign_keys_;
      }

      const std::string&
      vfs () const
      {
        return vfs_;
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
      erase_query (const sqlite::query_base&);

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
      query (const sqlite::query_base&);

      template <typename T>
      result<T>
      query (const odb::query_base&);

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
      query_one (const sqlite::query_base&);

      template <typename T>
      bool
      query_one (const sqlite::query_base&, T& object);

      template <typename T>
      T
      query_value (const sqlite::query_base&);

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
      prepare_query (const char* name, const sqlite::query_base&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const odb::query_base&);

      // Transactions.
      //
    public:
      virtual transaction_impl*
      begin ();

      transaction_impl*
      begin_immediate ();

      transaction_impl*
      begin_exclusive ();

    public:
      connection_ptr
      connection ();

      // SQL statement tracing.
      //
    public:
      typedef sqlite::tracer tracer_type;

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

      // Database id constant (useful for meta-programming).
      //
    public:
      static const odb::database_id database_id = id_sqlite;

    public:
      virtual
      ~database ();

    protected:
      virtual odb::connection*
      connection_ ();

    private:
      friend class transaction_impl; // factory_

      // Note: remember to update move ctor if adding any new members.
      //
      std::string name_;
      std::string schema_;
      int flags_;
      bool foreign_keys_;
      std::string vfs_;

      // Note: keep last so that all other database members are still valid
      // during factory's destruction.
      //
      details::unique_ptr<connection_factory> factory_;
    };
  }
}

#include <odb/sqlite/database.ixx>

#include <odb/post.hxx>

#endif // ODB_SQLITE_DATABASE_HXX
