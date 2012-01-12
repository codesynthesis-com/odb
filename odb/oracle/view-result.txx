// file      : odb/oracle/view-result.txx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/callback.hxx>
#include <odb/exceptions.hxx>

#include <odb/oracle/view-statements.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T>
    view_result_impl<T>::
    ~view_result_impl ()
    {
      oracle::change_callback& cc (statements_.image ().change_callback_);

      if (cc.context == this)
      {
        cc.callback = 0;
        cc.context = 0;
      }

      delete image_copy_;
    }

    template <typename T>
    view_result_impl<T>::
    view_result_impl (const query&,
                      details::shared_ptr<select_statement> statement,
                      view_statements<view_type>& statements)
        : base_type (statements.connection ().database ()),
          statement_ (statement),
          statements_ (statements),
          use_copy_ (false),
          image_copy_ (0)
    {
    }

    template <typename T>
    void view_result_impl<T>::
    load (view_type& view)
    {
      odb::database& db (this->database ());

      view_traits::callback (db, view, callback_event::pre_load);

      view_traits::init (view,
                         use_copy_ ? *image_copy_ : statements_.image (),
                         db);

      statement_->stream_result ();
      view_traits::callback (db, view, callback_event::post_load);
    }

    template <typename T>
    void view_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      typename view_traits::image_type& im (statements_.image ());
      oracle::change_callback& cc (im.change_callback_);

      if (cc.context == this)
      {
        cc.callback = 0;
        cc.context = 0;
      }

      use_copy_ = false;

      if (im.version != statements_.image_version ())
      {
        binding& b (statements_.image_binding ());
        view_traits::bind (b.bind, im);
        statements_.image_version (im.version);
        b.version++;
      }

      if (statement_->fetch () == select_statement::no_data)
        this->end_ = true;
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

      // Increment binding version since we may have "stolen" some
      // descriptors (LOB, date-time) from the image. Re-bind will
      // reallocate them and update the binding.
      //
      r->statements_.image_binding ().version++;

      im.change_callback_.callback = 0;
      im.change_callback_.context = 0;

      r->use_copy_ = true;
    }
  }
}
