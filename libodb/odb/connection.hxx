// file      : odb/connection.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_CONNECTION_HXX
#define ODB_CONNECTION_HXX

#include <odb/pre.hxx>

#include <map>
#include <string>
#include <memory>   // std::unique_ptr, std::shared_ptr
#include <cstddef>  // std::size_t
#include <typeinfo>

#include <odb/forward.hxx>
#include <odb/traits.hxx>
#include <odb/query.hxx>
#include <odb/prepared-query.hxx>

#include <odb/details/export.hxx>
#include <odb/details/c-string.hxx>

namespace odb
{
  class transaction_impl;
  class connection_factory;

  class connection;
  typedef std::shared_ptr<connection> connection_ptr;

  class LIBODB_EXPORT connection: public std::enable_shared_from_this<connection>
  {
  public:
    // The connection is neither move-constructible/assignable nor copy-
    // constructible/assignable.
    //
    connection (connection&&) = delete;
    connection (const connection&) = delete;
    connection& operator= (connection&&) = delete;
    connection& operator= (const connection&) = delete;

    typedef odb::database database_type;

    database_type&
    database ();

    // Transactions.
    //
  public:
    std::unique_ptr<transaction_impl>
    begin ();

    // Native database statement execution. Note that unlike the
    // versions in the database class, these can be executed
    // without a transaction.
    //
  public:
    unsigned long long
    execute (const char* statement);

    unsigned long long
    execute (const std::string& statement);

    virtual unsigned long long
    execute (const char* statement, std::size_t length) = 0;

    // Query preparation.
    //
  public:
    template <typename T>
    prepared_query<T>
    prepare_query (const char* name, const char*);

    template <typename T>
    prepared_query<T>
    prepare_query (const char* name, const std::string&);

    template <typename T>
    prepared_query<T>
    prepare_query (const char* name, const query<T>&);

    template <typename T>
    void
    cache_query (const prepared_query<T>&);

    template <typename T, typename P>
    void
    cache_query (const prepared_query<T>&, std::unique_ptr<P> params);

    template <typename T>
    prepared_query<T>
    lookup_query (const char* name);

    template <typename T, typename P>
    prepared_query<T>
    lookup_query (const char* name, P*& params);

    // SQL statement tracing.
    //
  public:
    typedef odb::tracer tracer_type;

    void
    tracer (tracer_type&);

    void
    tracer (tracer_type*);

    tracer_type*
    tracer () const;

   public:
    // Store the transaction-spacific tracer in the connection. If we
    // were to store it in the transaction, then in order to check if
    // it was set, we would need to get the transaction instance using
    // the current() API. But that requires a TLS lookup, which can be
    // slow.
    //
    tracer_type*
    transaction_tracer () const;

  public:
    virtual
    ~connection ();

    // Recycle the connection to be used by another thread. This call
    // invalidates uncached prepared queries.
    //
    void
    recycle ();

  protected:
    virtual std::unique_ptr<transaction_impl>
    begin_ () = 0;

  protected:
    connection (connection_factory&);

    template <typename T,
              database_id DB,
              class_kind kind = class_traits<T>::kind>
    struct query_;

    virtual void
    cache_query_ (std::shared_ptr<prepared_query_impl> pq,
                  const std::type_info& ti,
                  void* params,
                  const std::type_info* params_info,
                  void (*params_deleter) (void*));

    std::shared_ptr<prepared_query_impl>
    lookup_query_ (const char* name,
                   const std::type_info& ti,
                   void** params, // out
                   const std::type_info* params_info) const;

    template <typename P>
    static void
    params_deleter (void*);

    // Prepared query cache.
    //
  protected:
    struct prepared_entry_type
    {
      std::shared_ptr<prepared_query_impl> prep_query;
      const std::type_info* type_info;
      void* params;
      const std::type_info* params_info;
      void (*params_deleter) (void*);
    };

    typedef
    std::map<const char*, prepared_entry_type, details::c_string_comparator>
    prepared_map_type;

    prepared_map_type prepared_map_;

    void
    clear_prepared_map ();

    // Implementation details.
    //
  public:
    connection_factory& factory_;

  protected:
    tracer_type* tracer_;

    // Active query result list.
    //
  protected:
    friend class result_impl;
    result_impl* results_;

    void
    invalidate_results ();

    // Prepared but uncached doubly-linked query list (cached ones are stored
    // in prepared_map_).
    //
  protected:
    friend class prepared_query_impl;
    prepared_query_impl* uncached_prepared_queries_;

    // Implementation details.
    //
  public:
    tracer_type* transaction_tracer_;
  };

  class connection_factory
  {
  public:
    typedef odb::database database_type;

    connection_factory (): db_ (0) {}

    connection_factory (connection_factory&&) = delete;
    connection_factory (const connection_factory&) = delete;
    connection_factory& operator= (connection_factory&&) = delete;
    connection_factory& operator= (const connection_factory&) = delete;

    database_type&
    database () {return *db_;}

  protected:
    database_type* db_;
  };
}

#include <odb/connection.ixx>
#include <odb/connection.txx>

#include <odb/post.hxx>

#endif // ODB_CONNECTION_HXX
