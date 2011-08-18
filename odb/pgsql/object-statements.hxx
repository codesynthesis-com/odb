// file      : odb/pgsql/object-statements.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_OBJECT_STATEMENTS_HXX
#define ODB_PGSQL_OBJECT_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <vector>
#include <cassert>
#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>
#include <odb/cache-traits.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/binding.hxx>
#include <odb/pgsql/statement.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class connection;

    class LIBODB_PGSQL_EXPORT object_statements_base:
      public details::shared_base
    {
    public:
      typedef pgsql::connection connection_type;

      connection_type&
      connection ()
      {
        return conn_;
      }

      // Locking.
      //
      void
      lock ()
      {
        assert (!locked_);
        locked_ = true;
      }

      void
      unlock ()
      {
        assert (locked_);
        locked_ = false;
      }

      bool
      locked () const
      {
        return locked_;
      }

    public:
      virtual
      ~object_statements_base ();

    protected:
      object_statements_base (connection_type& conn)
        : conn_ (conn), locked_ (false)
      {
      }

      struct auto_unlock
      {
        // Unlocks the statement on construction and re-locks it on
        // destruction.
        //
        auto_unlock (object_statements_base&);
        ~auto_unlock ();

      private:
        auto_unlock (const auto_unlock&);
        auto_unlock& operator= (const auto_unlock&);

      private:
        object_statements_base& s_;
      };

    protected:
      connection_type& conn_;
      bool locked_;
    };

    template <typename T>
    class object_statements: public object_statements_base
    {
    public:
      typedef T object_type;
      typedef odb::object_traits<object_type> object_traits;
      typedef typename object_traits::id_type id_type;
      typedef typename object_traits::pointer_type pointer_type;
      typedef typename object_traits::image_type image_type;
      typedef typename object_traits::id_image_type id_image_type;

      typedef pointer_cache_traits<pointer_type> object_cache_traits;

      typedef
      typename object_traits::container_statement_cache_type
      container_statement_cache_type;

      typedef pgsql::insert_statement persist_statement_type;
      typedef pgsql::select_statement find_statement_type;
      typedef pgsql::update_statement update_statement_type;
      typedef pgsql::delete_statement erase_statement_type;

      // Automatic lock.
      //
      struct auto_lock
      {
        // Lock the statements unless they are already locked in which
        // case subsequent calls to locked() will return false.
        //
        auto_lock (object_statements&);

        // Unlock the statemens if we are holding the lock and clear
        // the delayed loads. This should only happen in case an
        // exception is thrown. In normal circumstances, the user
        // should call unlock() explicitly.
        //
        ~auto_lock ();

        // Return true if this auto_lock instance holds the lock.
        //
        bool
        locked () const;

        // Unlock the statemens.
        //
        void
        unlock ();

      private:
        auto_lock (const auto_lock&);
        auto_lock& operator= (const auto_lock&);

      private:
        object_statements& s_;
        bool locked_;
      };

      //
      //
      object_statements (connection_type&);

      // Delayed loading.
      //
      void
      delay_load (const id_type& id,
                  object_type& obj,
                  const typename object_cache_traits::position_type& p)
      {
        delayed_.push_back (delayed_load (id, obj, p));
      }

      void
      load_delayed ()
      {
        assert (locked ());

        if (!delayed_.empty ())
          load_delayed_ ();
      }

      void
      clear_delayed ()
      {
        if (!delayed_.empty ())
          clear_delayed_ ();
      }

      // Object image.
      //
      image_type&
      image ()
      {
        return image_;
      }

      std::size_t
      in_image_version () const
      {
        return in_image_version_;
      }

      std::size_t
      out_image_version () const
      {
        return out_image_version_;
      }

      void
      in_image_version (std::size_t v)
      {
        in_image_version_ = v;
      }

      void
      out_image_version (std::size_t v)
      {
        out_image_version_ = v;
      }

      binding&
      in_image_binding ()
      {
        return in_image_binding_;
      }

      binding&
      out_image_binding ()
      {
        return out_image_binding_;
      }

      bool*
      out_image_truncated ()
      {
        return out_image_truncated_;
      }

      // Object id image.
      //
      id_image_type&
      id_image ()
      {
        return id_image_;
      }

      std::size_t
      id_image_version () const
      {
        return id_image_version_;
      }

      void
      id_image_version (std::size_t v)
      {
        id_image_version_ = v;
      }

      binding&
      id_image_binding ()
      {
        return id_image_binding_;
      }

      // Statements.
      //
      persist_statement_type&
      persist_statement ()
      {
        if (persist_ == 0)
        {
          persist_.reset (
            new (details::shared) persist_statement_type (
              conn_,
              object_traits::persist_statement_name,
              object_traits::persist_statement,
              object_traits::persist_statement_types,
              object_traits::in_column_count,
              in_image_binding_,
              in_image_native_binding_));
        }

        return *persist_;
      }

      find_statement_type&
      find_statement ()
      {
        if (find_ == 0)
        {
          find_.reset (
            new (details::shared) find_statement_type (
              conn_,
              object_traits::find_statement_name,
              object_traits::find_statement,
              object_traits::find_statement_types,
              1,
              id_image_binding_,
              id_image_native_binding_,
              out_image_binding_));
        }

        return *find_;
      }

      update_statement_type&
      update_statement ()
      {
        if (update_ == 0)
        {
          update_.reset (
            new (details::shared) update_statement_type (
              conn_,
              object_traits::update_statement_name,
              object_traits::update_statement,
              object_traits::update_statement_types,
              object_traits::in_column_count + 1,
              id_image_binding_,
              id_image_native_binding_,
              in_image_binding_,
              in_image_native_binding_));
        }

        return *update_;
      }

      erase_statement_type&
      erase_statement ()
      {
        if (erase_ == 0)
        {
          erase_.reset (
            new (details::shared) erase_statement_type (
              conn_,
              object_traits::erase_statement_name,
              object_traits::erase_statement,
              object_traits::erase_statement_types,
              1,
              id_image_binding_,
              id_image_native_binding_));
        }

        return *erase_;
      }

      // Container statement cache.
      //
      container_statement_cache_type&
      container_statment_cache ()
      {
        return container_statement_cache_;
      }

    private:
      object_statements (const object_statements&);
      object_statements& operator= (const object_statements&);

    private:
      void
      load_delayed_ ();

      void
      clear_delayed_ ();

    private:
      container_statement_cache_type container_statement_cache_;

      image_type image_;

      // In (send) binding. The last element is the id parameter.
      //
      std::size_t in_image_version_;
      binding in_image_binding_;
      bind in_image_bind_[object_traits::in_column_count + 1];
      native_binding in_image_native_binding_;
      char* in_image_values_[object_traits::in_column_count + 1];
      int in_image_lengths_[object_traits::in_column_count + 1];
      int in_image_formats_[object_traits::in_column_count + 1];

      // Out (receive) binding.
      //
      std::size_t out_image_version_;
      binding out_image_binding_;
      bind out_image_bind_[object_traits::out_column_count];
      bool out_image_truncated_[object_traits::out_column_count];

      // Id image binding (only in).
      //
      id_image_type id_image_;
      std::size_t id_image_version_;
      binding id_image_binding_;
      native_binding id_image_native_binding_;

      details::shared_ptr<persist_statement_type> persist_;
      details::shared_ptr<find_statement_type> find_;
      details::shared_ptr<update_statement_type> update_;
      details::shared_ptr<erase_statement_type> erase_;

      // Delayed loading.
      //
      struct delayed_load
      {
        typedef typename object_cache_traits::position_type position_type;

        delayed_load () {}
        delayed_load (const id_type& i, object_type& o, const position_type& p)
            : id (i), obj (&o), pos (p)
        {
        }

        id_type id;
        object_type* obj;
        position_type pos;
      };

      typedef std::vector<delayed_load> delayed_loads;
      delayed_loads delayed_;

      // Delayed vectors swap guard. See the load_delayed_() function for
      // details.
      //
      struct swap_guard
      {
        swap_guard (object_statements& os, delayed_loads& dl)
            : os_ (os), dl_ (dl)
        {
          dl_.swap (os_.delayed_);
        }

        ~swap_guard ()
        {
          os_.clear_delayed ();
          dl_.swap (os_.delayed_);
        }

      private:
        object_statements& os_;
        delayed_loads& dl_;
      };
    };
  }
}

#include <odb/pgsql/object-statements.ixx>
#include <odb/pgsql/object-statements.txx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_OBJECT_STATEMENTS_HXX
