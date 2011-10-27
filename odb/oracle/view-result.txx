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
    }

    template <typename T>
    view_result_impl<T>::
    view_result_impl (const query&,
                      details::shared_ptr<select_statement> statement,
                      view_statements<view_type>& statements)
        : base_type (statements.connection ().database ()),
          statement_ (statement),
          statements_ (statements)
    {
    }

    template <typename T>
    void view_result_impl<T>::
    load (view_type& view)
    {
      odb::database& db (this->database ());

      view_traits::callback (db, view, callback_event::pre_load);
      view_traits::init (view, statements_.image (), db);
      statement_->stream_result ();
      view_traits::callback (db, view, callback_event::post_load);
    }

    template <typename T>
    void view_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      typename view_traits::image_type& im (statements_.image ());

      if (im.version != statements_.image_version ())
      {
        binding& b (statements_.image_binding ());
        view_traits::bind (b.bind, im);
        statements_.image_version (im.version);
        b.version++;
      }

      if (statement_->fetch () == select_statement::no_data)
        this->end_ = true;
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
  }
}
