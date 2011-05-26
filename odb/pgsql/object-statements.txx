// file      : odb/pgsql/object-statements.txx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset

#include <odb/session.hxx>
#include <odb/exceptions.hxx>

#include <odb/pgsql/connection.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename T>
    object_statements<T>::
    object_statements (connection_type& conn)
        : object_statements_base (conn),
          container_statement_cache_ (conn),
          in_image_binding_ (in_image_bind_, object_traits::in_column_count),
          in_image_native_binding_ (in_image_values_, in_image_lengths_, in_image_formats_, object_traits::in_column_count)
          out_image_binding_ (out_image_bind_, object_traits::out_column_count),
          id_image_binding_ (in_image_bind_ + object_traits::in_column_count, 1),
          id_image_native_binding_ (in_image_values_ + object_traits::in_column_count, in_image_lengths_ + object_traits::in_column_count, in_image_formats_ + object_traits::in_column_count, 1)
    {
      image_.version = 0;
      in_image_version_ = 0;
      out_image_version_ = 0;

      id_image_.version = 0;
      id_image_version_ = 0;

      std::memset (in_image_bind_, 0, sizeof (in_image_bind_));
      std::memset (out_image_bind_, 0, sizeof (out_image_bind_));
      std::memset (out_image_truncated_, 0, sizeof (out_image_truncated_));

      // @@ Is it necessary to zero native array contents?
      //

      for (std::size_t i (0); i < object_traits::out_column_count; ++i)
        out_image_bind_[i].truncated = out_image_truncated_ + i;
    }

    template <typename T>
    void object_statements<T>::
    load_delayed_ ()
    {
      // We should be careful here: the delayed vector can change
      // from under us as a result of a recursive load.
      //
      database& db (connection ().database ());

      while (!delayed_.empty ())
      {
        delayed_load l (delayed_.back ());
        typename object_cache_traits::insert_guard g (l.pos);
        delayed_.pop_back ();

        if (!object_traits::find_ (*this, l.id))
          throw object_not_persistent ();

        object_traits::init (*l.obj, image (), db);
        object_traits::load_ (*this, *l.obj); // Load containers, etc.
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
