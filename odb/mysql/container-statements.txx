// file      : odb/mysql/container-statements.txx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset

namespace odb
{
  namespace mysql
  {
    // container_statements
    //

    template <typename T>
    container_statements<T>::
    container_statements (connection_type& conn)
        : conn_ (conn),
          functions_ (this,
                      &traits::insert_one,
                      &traits::load_all,
                      &traits::delete_all),
          id_binding_ (0),
          cond_image_binding_ (0, 0),  // Initialized by impl.
          data_image_binding_ (0, 0),  // Initialized by impl.
          select_image_binding_ (0, 0) // Initialized by impl.
    {
      cond_image_.version = 0;
      cond_image_version_ = 0;
      cond_id_binding_version_ = 0;

      data_image_.version = 0;
      data_image_version_ = 0;
      data_id_binding_version_ = 0;
    }

    template <typename T>
    container_statements_impl<T>::
    container_statements_impl (connection_type& conn)
        : base (conn)
    {
      this->select_image_truncated_ = select_image_truncated_array_;

      this->cond_image_binding_.bind = cond_image_bind_;
      this->cond_image_binding_.count = traits::cond_column_count;

      this->data_image_binding_.bind = data_image_bind_;
      this->data_image_binding_.count = traits::data_column_count;

      this->select_image_binding_.bind = data_image_bind_ +
        traits::id_column_count;
      this->select_image_binding_.count = traits::data_column_count -
        traits::id_column_count;

      std::memset (cond_image_bind_, 0, sizeof (cond_image_bind_));
      std::memset (data_image_bind_, 0, sizeof (data_image_bind_));
      std::memset (select_image_truncated_array_,
                   0,
                   sizeof (select_image_truncated_array_));

      for (std::size_t i (0);
           i < traits::data_column_count - traits::id_column_count;
           ++i)
        data_image_bind_[i + traits::id_column_count].error =
          select_image_truncated_array_ + i;

      this->insert_one_text_ = traits::insert_one_statement;
      this->select_all_text_ = traits::select_all_statement;
      this->delete_all_text_ = traits::delete_all_statement;
    }
  }
}
