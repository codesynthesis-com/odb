// file      : odb/prepared-query.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PREPARED_QUERY_HXX
#define ODB_PREPARED_QUERY_HXX

#include <odb/pre.hxx>

#include <memory> // std::shared_ptr

#include <odb/forward.hxx> // odb::core
#include <odb/traits.hxx>
#include <odb/result.hxx>
#include <odb/statement.hxx>

#include <odb/details/export.hxx>

namespace odb
{
  class LIBODB_EXPORT prepared_query_impl
  {
  public:
    virtual
    ~prepared_query_impl ();

    prepared_query_impl (connection&);

    // Verify this prepared query and the specified transaction use the
    // same connection.
    //
    virtual bool
    verify_connection (transaction&);

    bool cached;
    connection& conn;
    const char* name;
    std::shared_ptr<statement> stmt;
    std::shared_ptr<result_impl> (*execute) (prepared_query_impl&);

  private:
    prepared_query_impl (const prepared_query_impl&);
    prepared_query_impl& operator= (const prepared_query_impl&);

    // Doubly-linked list of results.
    //
    // prev_ ==    0 means we are the first element.
    // next_ ==    0 means we are the last element.
    // next_ == this means we are not on the list.
    //
  protected:
    friend class connection;

    void
    list_remove ();

    prepared_query_impl* prev_;
    prepared_query_impl* next_;
  };

  template <typename T>
  struct prepared_query
  {
    prepared_query ()
        : impl_ ()
    {
    }

    explicit
    prepared_query (std::shared_ptr<prepared_query_impl> impl)
        : impl_ (std::move (impl))
    {
    }

    result<T>
    execute (bool cache = true)
    {
      typedef
      typename result_base<T, class_traits<T>::kind>::result_impl_type
      derived_type;

      result<T> r (
        std::static_pointer_cast<derived_type> (
          impl_->execute (*impl_)));

      if (cache)
        r.cache ();

      return r;
    }

    typename object_traits<T>::pointer_type
    execute_one ()
    {
      return execute (false).one ();
    }

    bool
    execute_one (T& object)
    {
      return execute (false).one (object);
    }

    T
    execute_value ()
    {
      // Compiler error pointing here? The object must be default-
      // constructible in order to use the return-by-value API.
      //
      T o;
      execute (false).value (o);
      return o;
    }

    const char*
    name () const
    {
      return impl_->name;
    }

    typedef odb::statement statement_type;

    statement_type&
    statement () const
    {
      return *impl_->stmt;
    }

    explicit operator bool () const
    {
      return impl_ != nullptr;
    }

    // Copying or assignment of a prepared query instance leads to one
    // instance being an alias for another. Think of copying a query as
    // copying a file handle -- the file you access through either of them is
    // still the same.
    //
  public:
    prepared_query (const prepared_query& x)
        : impl_ (x.impl_)
    {
    }

    prepared_query&
    operator= (const prepared_query& x)
    {
      if (impl_ != x.impl_)
        impl_ = x.impl_;

      return *this;
    }

    // @@ CXX11: add move ctor/assign.

  private:
    friend class connection;
    std::shared_ptr<prepared_query_impl> impl_;
  };

  namespace common
  {
    using odb::prepared_query;
  }
}

#include <odb/post.hxx>

#endif // ODB_PREPARED_QUERY_HXX
