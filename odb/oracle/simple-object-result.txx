// file      : odb/oracle/simple-object-result.txx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <odb/callback.hxx>
#include <odb/exceptions.hxx> // result_not_cached

#include <odb/oracle/simple-object-statements.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T>
    object_result_impl<T>::
    ~object_result_impl ()
    {
      invalidate ();
    }

    template <typename T>
    void object_result_impl<T>::
    invalidate ()
    {
      change_callback_type& cc (statements_.image ().change_callback_);

      if (cc.context == this)
      {
        cc.callback = 0;
        cc.context = 0;
      }

      delete image_copy_;
      image_copy_ = 0;

      if (!this->end_)
      {
        statement_->free_result ();
        this->end_ = true;
      }

      statement_.reset ();
    }

    template <typename T>
    object_result_impl<T>::
    object_result_impl (const query_base&,
                        details::shared_ptr<select_statement> statement,
                        statements_type& statements)
        : base_type (statements.connection ()),
          statement_ (statement),
          statements_ (statements),
          use_copy_ (false),
          image_copy_ (0)
    {
    }

    template <typename T>
    void object_result_impl<T>::
    load (object_type& obj, bool)
    {
      // This is a top-level call so the statements cannot be locked.
      //
      assert (!statements_.locked ());
      typename statements_type::auto_lock l (statements_);

      object_traits::callback (this->db_, obj, callback_event::pre_load);

      typename object_traits::image_type& i (
        use_copy_ ? *image_copy_ : statements_.image ());

      object_traits::init (obj, i, &this->db_);

      // If we are using a copy, make sure the callback information for
      // LOB data also comes from the copy.
      //
      statement_->stream_result (
        use_copy_ ? &statements_.image () : 0,
        use_copy_ ? image_copy_ : 0);

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
      object_traits::callback (this->db_, obj, callback_event::post_load);
    }

    template <typename T>
    typename object_result_impl<T>::id_type
    object_result_impl<T>::
    load_id ()
    {
      return object_traits::id (
        use_copy_ ? *image_copy_ : statements_.image ());
    }

    template <typename T>
    void object_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      typename object_traits::image_type& im (statements_.image ());
      change_callback_type& cc (im.change_callback_);

      if (cc.context == this)
      {
        cc.callback = 0;
        cc.context = 0;
      }

      use_copy_ = false;

      if (im.version != statements_.select_image_version ())
      {
        binding& b (statements_.select_image_binding ());
        object_traits::bind (b.bind, im, statement_select);
        statements_.select_image_version (im.version);
        b.version++;
      }

      if (statement_->fetch () == select_statement::no_data)
      {
        statement_->free_result ();
        this->end_ = true;
      }
      else
      {
        cc.callback = &change_callback;
        cc.context = this;
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
    change_callback (void* c, binding* b)
    {
      object_result_impl<T>* r (static_cast<object_result_impl<T>*> (c));
      typename object_traits::image_type& im (r->statements_.image ());

      if (r->image_copy_ == 0)
        r->image_copy_ = new typename object_traits::image_type (im);
      else
        *r->image_copy_ = im;

      // Increment image version since we may have "stolen" descriptors
      // (LOB, date-time) from the image. This will trigger re-bind which
      // will reallocate them and update the binding. In case this callback
      // was triggeted as part of a select statement fetch, then it is too
      // late to update the image version and we also need to update the
      // image binding.
      //
      // @@ Would be good to only do this if we actually have descriptors.
      //
      im.version++;
      if (b != 0)
        b->version++;

      im.change_callback_.callback = 0;
      im.change_callback_.context = 0;

      r->use_copy_ = true;
    }
  }
}
