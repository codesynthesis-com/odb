// file      : odb/oracle/object-statements.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_OBJECT_STATEMENTS_HXX
#define ODB_ORACLE_OBJECT_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <vector>
#include <cassert>
#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>
#include <odb/cache-traits.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-types.hxx>
#include <odb/oracle/oracle-fwd.hxx>   // sb2
#include <odb/oracle/statement.hxx>
#include <odb/oracle/statements-base.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class LIBODB_ORACLE_EXPORT object_statements_base: public statements_base
    {
      // Locking.
      //
    public:
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
        : statements_base (conn), locked_ (false)
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

      typedef oracle::insert_statement insert_statement_type;
      typedef oracle::select_statement select_statement_type;
      typedef oracle::update_statement update_statement_type;
      typedef oracle::delete_statement delete_statement_type;

      // Automatic lock.
      //
      struct auto_lock
      {
        // Lock the statements unless they are already locked in which
        // case subsequent calls to locked() will return false.
        //
        auto_lock (object_statements&);

        // Unlock the statements if we are holding the lock and clear
        // the delayed loads. This should only happen in case an
        // exception is thrown. In normal circumstances, the user
        // should call unlock() explicitly.
        //
        ~auto_lock ();

        // Return true if this auto_lock instance holds the lock.
        //
        bool
        locked () const;

        // Unlock the statements.
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

    public:
      object_statements (connection_type&);

      virtual
      ~object_statements ();

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

      // Insert binding.
      //
      std::size_t
      insert_image_version () const { return insert_image_version_;}

      void
      insert_image_version (std::size_t v) {insert_image_version_ = v;}

      binding&
      insert_image_binding () {return insert_image_binding_;}

      // Update binding.
      //
      std::size_t
      update_image_version () const { return update_image_version_;}

      void
      update_image_version (std::size_t v) {update_image_version_ = v;}

      binding&
      update_image_binding () {return update_image_binding_;}

      // Select binding.
      //
      std::size_t
      select_image_version () const { return select_image_version_;}

      void
      select_image_version (std::size_t v) {select_image_version_ = v;}

      binding&
      select_image_binding () {return select_image_binding_;}

      // Object id image and binding.
      //
      id_image_type&
      id_image () {return id_image_;}

      std::size_t
      id_image_version () const {return id_image_version_;}

      void
      id_image_version (std::size_t v) {id_image_version_ = v;}

      binding&
      id_image_binding () {return id_image_binding_;}

      // Statements.
      //
      insert_statement_type&
      persist_statement ()
      {
        if (persist_ == 0)
          persist_.reset (
            new (details::shared) insert_statement_type (
              conn_,
              object_traits::persist_statement,
              insert_image_binding_,
              object_traits::auto_id));

        return *persist_;
      }

      select_statement_type&
      find_statement ()
      {
        if (find_ == 0)
          find_.reset (
            new (details::shared) select_statement_type (
              conn_,
              object_traits::find_statement,
              id_image_binding_,
              select_image_binding_,
              4096));  // Hardcode a 4kB LOB prefetch size.

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
              update_image_binding_));

        return *update_;
      }

      delete_statement_type&
      erase_statement ()
      {
        if (erase_ == 0)
          erase_.reset (
            new (details::shared) delete_statement_type (
              conn_,
              object_traits::erase_statement,
              id_image_binding_));

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
      // select = total
      // insert = total - inverse
      // update = total - inverse - id - readonly
      //
      static const std::size_t select_column_count =
        object_traits::column_count;

      static const std::size_t insert_column_count =
        object_traits::column_count - object_traits::inverse_column_count;

      static const std::size_t update_column_count = insert_column_count -
        object_traits::id_column_count - object_traits::readonly_column_count;

      static const std::size_t id_column_count =
        object_traits::id_column_count;

    private:
      container_statement_cache_type container_statement_cache_;

      image_type image_;

      // Select binding.
      //
      std::size_t select_image_version_;
      binding select_image_binding_;
      bind select_image_bind_[select_column_count];

      // Insert binding.
      //
      std::size_t insert_image_version_;
      binding insert_image_binding_;
      bind insert_image_bind_[insert_column_count];

      // Update binding. The suffix of the bind array is object id. We
      // do it this way for consistency with other database runtimes.
      //
      std::size_t update_image_version_;
      binding update_image_binding_;
      bind update_image_bind_[update_column_count + id_column_count];

      // Id image binding (only used as a parameter). Uses the suffix in
      // the update bind.
      //
      id_image_type id_image_;
      std::size_t id_image_version_;
      binding id_image_binding_;

      details::shared_ptr<insert_statement_type> persist_;
      details::shared_ptr<select_statement_type> find_;
      details::shared_ptr<update_statement_type> update_;
      details::shared_ptr<delete_statement_type> erase_;

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

#include <odb/oracle/object-statements.ixx>
#include <odb/oracle/object-statements.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_OBJECT_STATEMENTS_HXX
