// file      : odb/oracle/simple-object-statements.txx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::memset

#include <odb/session.hxx>
#include <odb/callback.hxx>
#include <odb/exceptions.hxx>

#include <odb/oracle/connection.hxx>

namespace odb
{
  namespace oracle
  {
    //
    // optimistic_data
    //

    template <typename T>
    optimistic_data<T, true>::
    optimistic_data (bind* b)
        : id_image_binding_ (
            b,
            object_traits::id_column_count +
            object_traits::managed_optimistic_column_count)
    {
      id_image_version_ = 0;
    }

    //
    // object_statements
    //

    template <typename T>
    object_statements<T>::
    ~object_statements ()
    {
    }

    template <typename T>
    object_statements<T>::
    object_statements (connection_type& conn)
        : object_statements_base (conn),
          container_statement_cache_ (conn),
          select_image_binding_ (select_image_bind_, select_column_count),
          insert_image_binding_ (insert_image_bind_, insert_column_count),
          update_image_binding_ (update_image_bind_,
                                 update_column_count + id_column_count +
                                 managed_optimistic_column_count),
          id_image_binding_ (update_image_bind_ + update_column_count,
                             id_column_count),
          od_ (update_image_bind_ + update_column_count)
    {
      image_.version = 0;
      select_image_version_ = 0;
      insert_image_version_ = 0;
      update_image_version_ = 0;
      update_id_image_version_ = 0;

      id_image_.version = 0;
      id_image_version_ = 0;

      select_image_binding_.change_callback = image_.change_callback ();

      std::memset (insert_image_bind_, 0, sizeof (insert_image_bind_));
      std::memset (update_image_bind_, 0, sizeof (update_image_bind_));
      std::memset (select_image_bind_, 0, sizeof (select_image_bind_));
    }

    template <typename T>
    void object_statements<T>::
    load_delayed_ ()
    {
      database& db (connection ().database ());

      delayed_loads dls;
      swap_guard sg (*this, dls);

      while (!dls.empty ())
      {
        delayed_load l (dls.back ());
        typename pointer_cache_traits::insert_guard g (l.pos);
        dls.pop_back ();

        if (l.loader == 0)
        {
          if (!object_traits::find_ (*this, &l.id))
            throw object_not_persistent ();

          object_traits::callback (db, *l.obj, callback_event::pre_load);

          // Our calls to init/load below can result in additional delayed
          // loads being added to the delayed_ vector. We need to process
          // those before we call the post callback.
          //
          object_traits::init (*l.obj, image (), &db);
          find_->stream_result ();
          object_traits::load_ (*this, *l.obj); // Load containers, etc.

          if (!delayed_.empty ())
            load_delayed_ ();

          // Temporarily unlock the statement for the post_load call so that
          // it can load objects of this type recursively. This is safe to do
          // because we have completely loaded the current object. Also the
          // delayed_ list is clear before the unlock and should be clear on
          // re-lock (since a callback can only call public API functions
          // which will make sure all the delayed loads are processed before
          // returning).
          //
          {
            auto_unlock u (*this);
            object_traits::callback (db, *l.obj, callback_event::post_load);
          }
        }
        else
          l.loader (db, l.id, *l.obj);

        g.release ();
      }
    }

    template <typename T>
    void object_statements<T>::
    clear_delayed_ ()
    {
      // Remove the objects from the session cache.
      //
      if (session::has_current ())
      {
        for (typename delayed_loads::iterator i (delayed_.begin ()),
               e (delayed_.end ()); i != e; ++i)
        {
          pointer_cache_traits::erase (i->pos);
        }
      }

      delayed_.clear ();
    }
  }
}
