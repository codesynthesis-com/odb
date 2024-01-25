// file      : odb/mysql/view-result.txx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/callback.hxx>
#include <odb/exceptions.hxx>

#include <odb/mysql/view-statements.hxx>

namespace odb
{
  namespace mysql
  {
    template <typename T>
    view_result_impl<T>::
    ~view_result_impl ()
    {
      if (!this->end_)
        statement_->free_result ();
    }

    template <typename T>
    void view_result_impl<T>::
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
    view_result_impl<T>::
    view_result_impl (const query_base&,
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
    void view_result_impl<T>::
    load (view_type& view)
    {
      if (count_ > statement_->fetched ())
        fetch ();

      view_traits::callback (this->db_, view, callback_event::pre_load);
      tc_.init (view, statements_.image (), &this->db_);
      view_traits::callback (this->db_, view, callback_event::post_load);
    }

    template <typename T>
    void view_result_impl<T>::
    next ()
    {
      this->current (pointer_type ());

      if (this->end_)
        return;

      // If we are cached, simply increment the position and
      // postpone the actual row fetching until later. This way
      // if the same view is loaded in between iteration, the
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
    void view_result_impl<T>::
    fetch ()
    {
      // If the result is cached, the image can grow between calls
      // to fetch() as a result of other statements execution.
      //
      if (statement_->cached ())
      {
        typename view_traits::image_type& im (statements_.image ());

        if (im.version != statements_.image_version ())
        {
          binding& b (statements_.image_binding ());
          tc_.bind (b.bind, im);
          statements_.image_version (im.version);
          b.version++;
        }
      }

      while (!this->end_ && count_ > statement_->fetched ())
      {
        select_statement::result r (statement_->fetch ());

        switch (r)
        {
        case select_statement::truncated:
          {
            // Don't re-fetch data we are skipping.
            //
            if (count_ != statement_->fetched ())
              continue;

            typename view_traits::image_type& im (statements_.image ());

            if (tc_.grow (im, statements_.image_truncated ()))
              im.version++;

            if (im.version != statements_.image_version ())
            {
              binding& b (statements_.image_binding ());
              tc_.bind (b.bind, im);
              statements_.image_version (im.version);
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
      }
    }

    template <typename T>
    void view_result_impl<T>::
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
    std::size_t view_result_impl<T>::
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
