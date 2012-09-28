// file      : odb/pgsql/no-id-object-result.txx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/callback.hxx>

#include <odb/pgsql/no-id-object-statements.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename T>
    no_id_object_result_impl<T>::
    ~no_id_object_result_impl ()
    {
      if (!this->end_)
        statement_->free_result ();
    }

    template <typename T>
    no_id_object_result_impl<T>::
    no_id_object_result_impl (const query&,
                              details::shared_ptr<select_statement> statement,
                              statements_type& statements)
        : base_type (statements.connection ().database ()),
          statement_ (statement),
          statements_ (statements),
          count_ (0)
    {
    }

    template <typename T>
    void no_id_object_result_impl<T>::
    load (object_type& obj)
    {
      // The image can grow between calls to load() as a result of other
      // statements execution.
      //
      typename object_traits::image_type& im (statements_.image ());

      if (im.version != statements_.select_image_version ())
      {
        binding& b (statements_.select_image_binding ());
        object_traits::bind (b.bind, im, statement_select);
        statements_.select_image_version (im.version);
        b.version++;
      }

      select_statement::result r (statement_->load ());

      if (r == select_statement::truncated)
      {
        if (object_traits::grow (im, statements_.select_image_truncated ()))
          im.version++;

        if (im.version != statements_.select_image_version ())
        {
          binding& b (statements_.select_image_binding ());
          object_traits::bind (b.bind, im, statement_select);
          statements_.select_image_version (im.version);
          b.version++;
          statement_->reload ();
        }
      }

      odb::database& db (this->database ());

      object_traits::callback (db, obj, callback_event::pre_load);
      object_traits::init (obj, im, &db);
      object_traits::callback (db, obj, callback_event::post_load);
    }

    template <typename T>
    void no_id_object_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      if (statement_->next ())
        count_++;
      else
      {
        statement_->free_result ();
        this->end_ = true;
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
      return this->end_ ? count_ : statement_->result_size ();
    }
  }
}
