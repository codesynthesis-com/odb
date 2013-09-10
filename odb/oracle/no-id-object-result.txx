// file      : odb/oracle/no-id-object-result.txx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/callback.hxx>
#include <odb/exceptions.hxx> // result_not_cached

#include <odb/oracle/no-id-object-statements.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T>
    no_id_object_result_impl<T>::
    ~no_id_object_result_impl ()
    {
      invalidate ();
    }

    template <typename T>
    void no_id_object_result_impl<T>::
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
    no_id_object_result_impl<T>::
    no_id_object_result_impl (const query_base&,
                              details::shared_ptr<select_statement> statement,
                              statements_type& statements,
                              const schema_version_migration* svm)
        : base_type (statements.connection ()),
          statement_ (statement),
          statements_ (statements),
          tc_ (svm),
          use_copy_ (false),
          image_copy_ (0)
    {
    }

    template <typename T>
    void no_id_object_result_impl<T>::
    load (object_type& obj)
    {
      object_traits::callback (this->db_, obj, callback_event::pre_load);

      tc_.init (obj,
                use_copy_ ? *image_copy_ : statements_.image (),
                &this->db_);

      // If we are using a copy, make sure the callback information for
      // LOB data also comes from the copy.
      //
      statement_->stream_result (
        use_copy_ ? &statements_.image () : 0,
        use_copy_ ? image_copy_ : 0);

      object_traits::callback (this->db_, obj, callback_event::post_load);
    }

    template <typename T>
    void no_id_object_result_impl<T>::
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
        tc_.bind (b.bind, im, statement_select);
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
    void no_id_object_result_impl<T>::
    cache ()
    {
    }

    template <typename T>
    std::size_t no_id_object_result_impl<T>::
    size ()
    {
      throw result_not_cached ();
    }

    template <typename T>
    void no_id_object_result_impl<T>::
    change_callback (void* c, binding* b)
    {
      no_id_object_result_impl<T>* r (
        static_cast<no_id_object_result_impl<T>*> (c));

      typename object_traits::image_type im (r->statements_.image ());

      if (r->image_copy_ == 0)
        r->image_copy_ = new typename object_traits::image_type (im);
      else
        *r->image_copy_ = im;

      // See comment in simple object_result for details on what's going
      // on here.
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
