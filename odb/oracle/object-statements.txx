// file      : odb/oracle/object-statements.txx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset

#include <odb/session.hxx>
#include <odb/callback.hxx>
#include <odb/exceptions.hxx>

#include <odb/oracle/connection.hxx>

namespace odb
{
  namespace oracle
  {
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
          in_image_binding_ (in_image_bind_, object_traits::in_column_count),
          out_image_binding_ (out_image_bind_, object_traits::out_column_count),
          id_image_binding_ (in_image_bind_ + object_traits::in_column_count, 1)
    {
      image_.version = 0;
      in_image_version_ = 0;
      out_image_version_ = 0;

      id_image_.version = 0;
      id_image_version_ = 0;

      std::memset (in_image_bind_, 0, sizeof (in_image_bind_));
      std::memset (in_image_indicator_, 0, sizeof (in_image_indicator_));
      std::memset (out_image_bind_, 0, sizeof (out_image_bind_));
      std::memset (out_image_indicator_, 0, sizeof (out_image_indicator_));

      for (std::size_t i (0); i < object_traits::in_column_count + 1; ++i)
        in_image_bind_[i].indicator = in_image_indicator_ + i;

      for (std::size_t i (0); i < object_traits::out_column_count; ++i)
        out_image_bind_[i].indicator = out_image_indicator_ + i;
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
        typename object_cache_traits::insert_guard g (l.pos);
        dls.pop_back ();

        if (!object_traits::find_ (*this, l.id))
          throw object_not_persistent ();

        object_traits::callback (db, *l.obj, callback_event::pre_load);

        // Our calls to init/load below can result in additional delayed
        // loads being added to the delayed_ vector. We need to process
        // those before we call the post callback.
        //
        object_traits::init (*l.obj, image (), db);
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
          object_cache_traits::erase (i->pos);
        }
      }

      delayed_.clear ();
    }
  }
}
