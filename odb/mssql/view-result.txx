// file      : odb/mssql/view-result.txx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/callback.hxx>
#include <odb/exceptions.hxx> // result_not_cached

#include <odb/mssql/exceptions.hxx> // long_data_reload
#include <odb/mssql/view-statements.hxx>

namespace odb
{
  namespace mssql
  {
    template <typename T>
    view_result_impl<T>::
    ~view_result_impl ()
    {
      invalidate ();
    }

    template <typename T>
    void view_result_impl<T>::
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
    view_result_impl<T>::
    view_result_impl (const query_base&,
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
    void view_result_impl<T>::
    load (view_type& view)
    {
      if (!can_load_)
        throw long_data_reload ();

      view_traits::callback (this->db_, view, callback_event::pre_load);

      tc_.init (view,
                use_copy_ ? *image_copy_ : statements_.image (),
                &this->db_);

      // If we are using a copy, make sure the callback information for
      // long data also comes from the copy.
      //
      can_load_ = !statement_->stream_result (
        use_copy_ ? &statements_.image () : 0,
        use_copy_ ? image_copy_ : 0);

      view_traits::callback (this->db_, view, callback_event::post_load);
    }

    template <typename T>
    void view_result_impl<T>::
    next ()
    {
      can_load_ = true;
      this->current (pointer_type ());

      typename view_traits::image_type& im (statements_.image ());
      change_callback_type& cc (im.change_callback_);

      if (cc.context == this)
      {
        cc.callback = 0;
        cc.context = 0;
      }

      use_copy_ = false;

      if (im.version != statements_.image_version ())
      {
        binding& b (statements_.image_binding ());
        tc_.bind (b.bind, im);
        statements_.image_version (im.version);
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
    void view_result_impl<T>::
    cache ()
    {
    }

    template <typename T>
    std::size_t view_result_impl<T>::
    size ()
    {
      throw result_not_cached ();
    }

    template <typename T>
    void view_result_impl<T>::
    change_callback (void* c)
    {
      view_result_impl<T>* r (static_cast<view_result_impl<T>*> (c));

      typename view_traits::image_type& im (r->statements_.image ());

      if (r->image_copy_ == 0)
        r->image_copy_ = new typename view_traits::image_type (im);
      else
        *r->image_copy_ = im;

      im.change_callback_.callback = 0;
      im.change_callback_.context = 0;

      r->use_copy_ = true;
    }
  }
}
