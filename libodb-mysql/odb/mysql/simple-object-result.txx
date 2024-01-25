// file      : odb/mysql/simple-object-result.txx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

#include <odb/callback.hxx>
#include <odb/exceptions.hxx> // result_not_cached

#include <odb/mysql/simple-object-statements.hxx>

namespace odb
{
  namespace mysql
  {
    template <typename T>
    object_result_impl<T>::
    ~object_result_impl ()
    {
      if (!this->end_)
        statement_->free_result ();
    }

    template <typename T>
    void object_result_impl<T>::
    invalidate ()
    {
      if (!this->end_)
      {
        statement_->free_result ();
        this->end_ = true;
      }

      statement_.reset ();
    }

    template <typename T>
    object_result_impl<T>::
    object_result_impl (const query_base&,
                        details::shared_ptr<select_statement> statement,
                        statements_type& statements,
                        const schema_version_migration* svm)
        : base_type (statements.connection ()),
          statement_ (statement),
          statements_ (statements),
          tc_ (svm),
          count_ (0)
    {
    }

    template <typename T>
    void object_result_impl<T>::
    load (object_type& obj, bool f)
    {
      if (count_ > statement_->fetched ())
        fetch ();
      else if (f && statement_->cached ())
      {
        // We have to re-load the image in case it has been overwritten
        // between the last time we fetched and this call to load().
        //
        fetch (false);
      }

      // This is a top-level call so the statements cannot be locked.
      //
      assert (!statements_.locked ());
      typename statements_type::auto_lock l (statements_);

      object_traits::callback (this->db_, obj, callback_event::pre_load);

      typename object_traits::image_type& i (statements_.image ());
      tc_.init (obj, i, &this->db_);

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

      tc_.load_ (statements_, obj, false);
      statements_.load_delayed (tc_.version ());
      l.unlock ();
      object_traits::callback (this->db_, obj, callback_event::post_load);
    }

    template <typename T>
    typename object_result_impl<T>::id_type
    object_result_impl<T>::
    load_id ()
    {
      if (count_ > statement_->fetched ())
        fetch ();
      else if (statement_->cached ())
      {
        // We have to re-load the image in case it has been overwritten
        // between the last time we fetched and this call to load_id().
        //
        fetch (false);
      }

      return object_traits::id (statements_.image ());
    }

    template <typename T>
    void object_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      if (this->end_)
        return;

      // If we are cached, simply increment the position and
      // postpone the actual row fetching until later. This way
      // if the same object is loaded in between iteration, the
      // image won't be messed up.
      //
      count_++;

      if (statement_->cached ())
        this->end_ = count_ > statement_->result_size ();
      else
        fetch ();

      if (this->end_)
        statement_->free_result ();
    }

    template <typename T>
    void object_result_impl<T>::
    fetch (bool next)
    {
      // If the result is cached, the image can grow between calls
      // to fetch() as a result of other statements execution.
      //
      if (statement_->cached ())
      {
        typename object_traits::image_type& im (statements_.image ());

        if (im.version != statements_.select_image_version ())
        {
          binding& b (statements_.select_image_binding ());
          tc_.bind (b.bind, im, statement_select);
          statements_.select_image_version (im.version);
          b.version++;
        }
      }

      while (!this->end_ && (!next || count_ > statement_->fetched ()))
      {
        select_statement::result r (statement_->fetch (next));

        switch (r)
        {
        case select_statement::truncated:
          {
            // Don't re-fetch data we are skipping.
            //
            if (next && count_ != statement_->fetched ())
              continue;

            typename object_traits::image_type& im (statements_.image ());

            if (tc_.grow (im, statements_.select_image_truncated ()))
              im.version++;

            if (im.version != statements_.select_image_version ())
            {
              binding& b (statements_.select_image_binding ());
              tc_.bind (b.bind, im, statement_select);
              statements_.select_image_version (im.version);
              b.version++;
              statement_->refetch ();
            }
            // Fall throught.
          }
        case select_statement::success:
          {
            break;
          }
        case select_statement::no_data:
          {
            this->end_ = true;
            break;
          }
        }

        // If we are refetching the current row, then we are done.
        //
        if (!next)
          break;
      }
    }

    template <typename T>
    void object_result_impl<T>::
    cache ()
    {
      if (!this->end_ && !statement_->cached ())
      {
        statement_->cache ();

        if (count_ == statement_->result_size ())
        {
          statement_->free_result ();
          count_++; // One past the result size.
          this->end_ = true;
        }
      }
    }

    template <typename T>
    std::size_t object_result_impl<T>::
    size ()
    {
      if (!this->end_)
      {
        if (!statement_->cached ())
          throw result_not_cached ();

        return statement_->result_size ();
      }
      else
        return count_ - 1; // One past the result size.
    }
  }
}
