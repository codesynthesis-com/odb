// file      : odb/oracle/object-result.txx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/callback.hxx>
#include <odb/exceptions.hxx>

#include <odb/oracle/object-statements.hxx>

namespace odb
{
  namespace oracle
  {
    //
    // object_result_impl
    //

    template <typename T>
    object_result_impl<T>::
    ~object_result_impl ()
    {
      statements_.image ().change_callback_.callback = 0;
      delete image_copy_;
    }

    template <typename T>
    object_result_impl<T>::
    object_result_impl (const query&,
                        details::shared_ptr<select_statement> statement,
                        object_statements<object_type>& statements)
        : base_type (statements.connection ().database ()),
          statement_ (statement),
          statements_ (statements),
          use_copy_ (false),
          image_copy_ (0)
    {
    }

    template <typename T>
    void object_result_impl<T>::
    load (object_type& obj)
    {
      // This is a top-level call so the statements cannot be locked.
      //
      assert (!statements_.locked ());
      typename object_statements<object_type>::auto_lock l (statements_);

      odb::database& db (this->database ());
      object_traits::callback (db, obj, callback_event::pre_load);

      typename object_traits::image_type& i (
        use_copy_ ? *image_copy_ : statements_.image ());

      object_traits::init (obj, i, db);
      statement_->stream_result ();

      // Initialize the id image and binding and load the rest of the object
      // (containers, etc).
      //
      typename object_traits::id_image_type& idi (statements_.id_image ());
      object_traits::init (idi, object_traits::id (i));

      binding& idb (statements_.id_image_binding ());
      if (idi.version != statements_.id_image_version () || idb.version == 0)
      {
        object_traits::bind (idb.bind, idi);
        statements_.id_image_version (idi.version);
        idb.version++;
      }

      object_traits::load_ (statements_, obj);
      statements_.load_delayed ();
      l.unlock ();
      object_traits::callback (db, obj, callback_event::post_load);
    }

    template <typename T>
    typename object_result_impl<T>::id_type
    object_result_impl<T>::
    load_id ()
    {
      return object_traits::id (statements_.image ());
    }

    template <typename T>
    void object_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      typename object_traits::image_type& im (statements_.image ());

      im.change_callback_.callback = 0;
      use_copy_ = false;

      if (im.version != statements_.select_image_version ())
      {
        binding& b (statements_.select_image_binding ());
        object_traits::bind (b.bind, im, statement_select);
        statements_.select_image_version (im.version);
        b.version++;
      }

      if (statement_->fetch () == select_statement::no_data)
        this->end_ = true;
      else
      {
        im.change_callback_.callback = &change_callback;
        im.change_callback_.context = this;
      }
    }

    template <typename T>
    void object_result_impl<T>::
    cache ()
    {
    }

    template <typename T>
    std::size_t object_result_impl<T>::
    size ()
    {
      throw result_not_cached ();
    }

    template <typename T>
    void object_result_impl<T>::
    change_callback (void* c)
    {
      object_result_impl<T>* r (static_cast<object_result_impl<T>*> (c));
      object_statements<object_type>& stmts (r->statements_);

      if (r->image_copy_ == 0)
        r->image_copy_ = new
          typename object_traits::image_type (stmts.image ());
      else
        *r->image_copy_ = stmts.image ();

      stmts.select_image_binding ().version++;
      stmts.image ().change_callback_.callback = 0;
      r->use_copy_ = true;
    }

    //
    // object_result_impl_no_id
    //

    template <typename T>
    object_result_impl_no_id<T>::
    ~object_result_impl_no_id ()
    {
      statements_.image ().change_callback_.callback = 0;
      delete image_copy_;
    }

    template <typename T>
    object_result_impl_no_id<T>::
    object_result_impl_no_id (const query&,
                              details::shared_ptr<select_statement> statement,
                              object_statements_no_id<object_type>& statements)
        : base_type (statements.connection ().database ()),
          statement_ (statement),
          statements_ (statements),
          use_copy_ (false),
          image_copy_ (0)
    {
    }

    template <typename T>
    void object_result_impl_no_id<T>::
    load (object_type& obj)
    {
      odb::database& db (this->database ());

      object_traits::callback (db, obj, callback_event::pre_load);

      if (use_copy_)
        object_traits::init (obj, *image_copy_, db);
      else
        object_traits::init (obj, statements_.image (), db);

      statement_->stream_result ();

      object_traits::callback (db, obj, callback_event::post_load);
    }

    template <typename T>
    void object_result_impl_no_id<T>::
    next ()
    {
      this->current (pointer_type ());

      typename object_traits::image_type& im (statements_.image ());

      im.change_callback_.callback = 0;
      use_copy_ = false;

      if (im.version != statements_.select_image_version ())
      {
        binding& b (statements_.select_image_binding ());
        object_traits::bind (b.bind, im, statement_select);
        statements_.select_image_version (im.version);
        b.version++;
      }

      if (statement_->fetch () == select_statement::no_data)
        this->end_ = true;
      else
      {
        im.change_callback_.callback = &change_callback;
        im.change_callback_.context = this;
      }
    }

    template <typename T>
    void object_result_impl_no_id<T>::
    cache ()
    {
    }

    template <typename T>
    std::size_t object_result_impl_no_id<T>::
    size ()
    {
      throw result_not_cached ();
    }

    template <typename T>
    void object_result_impl_no_id<T>::
    change_callback (void* c)
    {
      object_result_impl_no_id<T>* r (
        static_cast<object_result_impl_no_id<T>*> (c));

      object_statements_no_id<object_type>& stmts (r->statements_);

      if (r->image_copy_ == 0)
        r->image_copy_ = new
          typename object_traits::image_type (stmts.image ());
      else
        *r->image_copy_ = stmts.image ();

      stmts.select_image_binding ().version++;
      stmts.image ().change_callback_.callback = 0;
      r->use_copy_ = true;
    }
  }
}
