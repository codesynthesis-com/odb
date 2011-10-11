// file      : odb/pgsql/object-result.txx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/callback.hxx>

#include <odb/pgsql/object-statements.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename T>
    result_impl<T, class_object>::
    ~result_impl ()
    {
    }

    template <typename T>
    result_impl<T, class_object>::
    result_impl (const query&,
                 details::shared_ptr<select_statement> statement,
                 object_statements<object_type>& statements)
        : base_type (statements.connection ().database ()),
          statement_ (statement),
          statements_ (statements)
    {
    }

    template <typename T>
    void result_impl<T, class_object>::
    load (object_type& obj)
    {
      load_image ();

      // This is a top-level call so the statements cannot be locked.
      //
      assert (!statements_.locked ());
      typename object_statements<object_type>::auto_lock l (statements_);

      odb::database& db (this->database ());
      object_traits::callback (db, obj, callback_event::pre_load);

      typename object_traits::image_type& i (statements_.image ());
      object_traits::init (obj, i, db);

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
    typename result_impl<T, class_object>::id_type
    result_impl<T, class_object>::
    load_id ()
    {
      load_image ();
      return object_traits::id (statements_.image ());
    }

    template <typename T>
    void result_impl<T, class_object>::
    next ()
    {
      this->current (pointer_type ());

      if (!statement_->next ())
        this->end_ = true;
    }

    template <typename T>
    void result_impl<T, class_object>::
    load_image ()
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
    }

    template <typename T>
    void result_impl<T, class_object>::
    cache ()
    {
    }

    template <typename T>
    std::size_t result_impl<T, class_object>::
    size ()
    {
      return statement_->result_size ();
    }
  }
}
