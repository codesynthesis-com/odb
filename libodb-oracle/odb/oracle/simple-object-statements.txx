// file      : odb/oracle/simple-object-statements.txx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::memset

#include <odb/callback.hxx>
#include <odb/exceptions.hxx>

#include <odb/oracle/connection.hxx>
#include <odb/oracle/traits-calls.hxx>

namespace odb
{
  namespace oracle
  {
    //
    // optimistic_data
    //

    template <typename T>
    optimistic_data<T, true>::
    optimistic_data (bind* b, std::size_t skip, sb4* status)
        : id_image_binding_ (
            b,
            object_traits::id_column_count +
            object_traits::managed_optimistic_column_count,
            object_traits::batch,
            skip,
            status)
    {
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
          select_image_binding_ (select_image_bind_, select_column_count),
          insert_image_binding_ (insert_image_bind_,
                                 insert_column_count,
                                 object_traits::batch,
                                 sizeof (images),
                                 status_),
          update_image_binding_ (update_image_bind_,
                                 update_column_count + id_column_count +
                                 managed_optimistic_column_count,
                                 object_traits::batch,
                                 sizeof (images),
                                 status_),
          id_image_binding_ (update_image_bind_ + update_column_count,
                             id_column_count,
                             object_traits::batch,
                             sizeof (images),
                             status_),
          od_ (update_image_bind_ + update_column_count,
               sizeof (images),
               status_)
    {
      // Only versions in the first element used.
      //
      images_[0].obj.version = 0;
      images_[0].id.version = 0;

      select_image_version_ = 0;
      insert_image_version_ = 0;
      update_image_version_ = 0;
      update_id_image_version_ = 0;
      id_image_version_ = 0;

      // SELECT statements only use the first element (no batches).
      //
      select_image_binding_.change_callback =
        images_[0].obj.change_callback ();

      std::memset (insert_image_bind_, 0, sizeof (insert_image_bind_));
      std::memset (update_image_bind_, 0, sizeof (update_image_bind_));
      std::memset (select_image_bind_, 0, sizeof (select_image_bind_));
    }

    template <typename T>
    template <typename STS>
    void object_statements<T>::
    load_delayed_ (const schema_version_migration* svm)
    {
      database& db (connection ().database ());

      delayed_loads dls;
      swap_guard sg (*this, dls);

      while (!dls.empty ())
      {
        delayed_load l (dls.back ());
        typename pointer_cache_traits::insert_guard ig (l.pos);
        dls.pop_back ();

        if (l.loader == 0)
        {
          object_traits_calls<T> tc (svm);

          if (!tc.find_ (static_cast<STS&> (*this), &l.id))
            throw object_not_persistent ();

          object_traits::callback (db, *l.obj, callback_event::pre_load);

          // Our calls to init/load below can result in additional delayed
          // loads being added to the delayed_ vector. We need to process
          // those before we call the post callback.
          //
          tc.init (*l.obj, image (), &db);
          find_->stream_result ();

          // Load containers, etc.
          //
          tc.load_ (static_cast<STS&> (*this), *l.obj, false);

          if (!delayed_.empty ())
            load_delayed_<STS> (svm);

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
          l.loader (db, l.id, *l.obj, svm);

        pointer_cache_traits::load (ig.position ());
        ig.release ();
      }
    }

    template <typename T>
    void object_statements<T>::
    clear_delayed_ ()
    {
      // Remove the objects from the session cache.
      //
      for (typename delayed_loads::iterator i (delayed_.begin ()),
             e (delayed_.end ()); i != e; ++i)
      {
        pointer_cache_traits::erase (i->pos);
      }

      delayed_.clear ();
    }
  }
}
