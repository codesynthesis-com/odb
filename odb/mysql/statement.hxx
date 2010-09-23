// file      : odb/mysql/statement.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_STATEMENT_HXX
#define ODB_MYSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <map>
#include <string>
#include <cstddef>  // std::size_t
#include <typeinfo>

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/version.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class connection;

    class LIBODB_MYSQL_EXPORT binding
    {
    public:
      binding (MYSQL_BIND* b, std::size_t n)
          : bind (b), count (n), version (0)
      {
      }

      MYSQL_BIND* bind;
      std::size_t count;
      std::size_t version;

    private:
      binding (const binding&);
      binding& operator= (const binding&);
    };

    class LIBODB_MYSQL_EXPORT statement: public details::shared_base
    {
    public:
      virtual
      ~statement () = 0;

      // Cancel the statement execution (e.g., result fetching) so
      // that another statement can be executed on the connection.
      //
      virtual void
      cancel ();

    protected:
      statement (connection&);

    protected:
      connection& conn_;
      MYSQL_STMT* stmt_;
    };

    class LIBODB_MYSQL_EXPORT query_statement: public statement
    {
    public:
      virtual
      ~query_statement ();

      query_statement (connection& conn,
                       const std::string& statement,
                       binding& image,
                       MYSQL_BIND* parameters);
      enum result
      {
        success,
        no_data,
        truncated
      };

      void
      execute ();

      void
      cache ();

      std::size_t
      result_size ();

      result
      fetch ();

      void
      refetch ();

      void
      free_result ();

      virtual void
      cancel ();

    private:
      query_statement (const query_statement&);
      query_statement& operator= (const query_statement&);

    private:
      bool end_;
      bool cached_;
      std::size_t rows_;

      binding& image_;
      std::size_t image_version_;

      MYSQL_BIND* parameters_;
    };

    class LIBODB_MYSQL_EXPORT persist_statement: public statement
    {
    public:
      virtual
      ~persist_statement ();

      persist_statement (connection& conn,
                         const std::string& statement,
                         binding& image);

      void
      execute ();

      unsigned long long
      id ()
      {
        return mysql_stmt_insert_id (stmt_);
      }

    private:
      persist_statement (const persist_statement&);
      persist_statement& operator= (const persist_statement&);

    private:
      binding& image_;
      std::size_t version_;
    };

    class LIBODB_MYSQL_EXPORT find_statement: public statement
    {
    public:
      virtual
      ~find_statement ();

      find_statement (connection& conn,
                      const std::string& statement,
                      binding& id,
                      binding& image);
      enum result
      {
        success,
        no_data,
        truncated
      };

      // You are expected to call free_result() if this function
      // returns success or truncated.
      //
      result
      execute ();

      void
      refetch ();

      void
      free_result ();

      void
      cancel ();

    private:
      find_statement (const find_statement&);
      find_statement& operator= (const find_statement&);

    private:
      binding& id_;
      std::size_t id_version_;

      binding& image_;
      std::size_t image_version_;
    };

    class LIBODB_MYSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection& conn,
                        const std::string& statement,
                        binding& id,
                        binding& image);
      void
      execute ();

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);

    private:
      binding& id_;
      std::size_t id_version_;

      binding& image_;
      std::size_t image_version_;
    };

    class LIBODB_MYSQL_EXPORT erase_statement: public statement
    {
    public:
      virtual
      ~erase_statement ();

      erase_statement (connection& conn,
                       const std::string& statement,
                       binding& id);

      void
      execute ();

    private:
      erase_statement (const erase_statement&);
      erase_statement& operator= (const erase_statement&);

    private:
      binding& id_;
      std::size_t version_;
    };

    // Statement cache.
    //

    class LIBODB_MYSQL_EXPORT object_statements_base:
      public details::shared_base
    {
    public:
      virtual
      ~object_statements_base ();

    protected:
      object_statements_base (connection& conn)
          : conn_ (conn)
      {
      }

    protected:
      connection& conn_;
    };

    template <typename T>
    class object_statements: public object_statements_base
    {
    public:
      typedef odb::object_traits<T> object_traits;
      typedef typename object_traits::image_type image_type;
      typedef typename object_traits::id_image_type id_image_type;

      typedef mysql::persist_statement persist_statement_type;
      typedef mysql::find_statement find_statement_type;
      typedef mysql::update_statement update_statement_type;
      typedef mysql::erase_statement erase_statement_type;

      object_statements (connection&);

      image_type&
      image ()
      {
        return image_;
      }

      binding&
      image_binding ()
      {
        return image_binding_;
      }

      my_bool*
      image_error ()
      {
        return image_error_;
      }

      id_image_type&
      id_image ()
      {
        return id_image_;
      }

      binding&
      id_image_binding ()
      {
        return id_image_binding_;
      }

      persist_statement_type&
      persist_statement ()
      {
        if (persist_ == 0)
          persist_.reset (
            new (details::shared) persist_statement_type (
              conn_, object_traits::persist_statement, image_binding_));

        return *persist_;
      }

      find_statement_type&
      find_statement ()
      {
        if (find_ == 0)
          find_.reset (
            new (details::shared) find_statement_type (
              conn_,
              object_traits::find_statement,
              id_image_binding_,
              image_binding_));

        return *find_;
      }

      update_statement_type&
      update_statement ()
      {
        if (update_ == 0)
          update_.reset (
            new (details::shared) update_statement_type (
              conn_,
              object_traits::update_statement,
              id_image_binding_,
              image_binding_));

        return *update_;
      }

      erase_statement_type&
      erase_statement ()
      {
        if (erase_ == 0)
          erase_.reset (
            new (details::shared) erase_statement_type (
              conn_,
              object_traits::erase_statement,
              id_image_binding_));

        return *erase_;
      }

    private:
      object_statements (const object_statements&);
      object_statements& operator= (const object_statements&);

    private:
      // The last element is the id parameter. The update statement
      // depends on this being one contiguous arrays.
      //
      MYSQL_BIND image_bind_[object_traits::column_count + 1];

      image_type image_;
      my_bool image_error_[object_traits::column_count];
      binding image_binding_;

      id_image_type id_image_;
      binding id_image_binding_;

      details::shared_ptr<persist_statement_type> persist_;
      details::shared_ptr<find_statement_type> find_;
      details::shared_ptr<update_statement_type> update_;
      details::shared_ptr<erase_statement_type> erase_;
    };

    struct LIBODB_MYSQL_EXPORT type_info_comparator
    {
      bool
      operator() (const std::type_info* x, const std::type_info* y) const
      {
        // XL C++ on AIX has buggy type_info::before() in that
        // it returns true for two different type_info objects
        // that happened to be for the same type.
        //
#if defined(__xlC__) && defined(_AIX)
        return *x != *y && x->before (*y);
#else
        return x->before (*y);
#endif
      }
    };

    class LIBODB_MYSQL_EXPORT statement_cache
    {
    public:
      statement_cache (connection& conn)
          : conn_ (conn)
      {
      }

      template <typename T>
      object_statements<T>&
      find ()
      {
        map::iterator i (map_.find (&typeid (T)));

        if (i != map_.end ())
          return static_cast<object_statements<T>&> (*i->second);

        details::shared_ptr<object_statements<T> > p (
          new (details::shared) object_statements<T> (conn_));

        map_.insert (map::value_type (&typeid (T), p));
        return *p;
      }

    private:
      typedef std::map<const std::type_info*,
                       details::shared_ptr<object_statements_base>,
                       type_info_comparator> map;

      connection& conn_;
      map map_;
    };
  }
}

#include <odb/mysql/statement.txx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_STATEMENT_HXX
