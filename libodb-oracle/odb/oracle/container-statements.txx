// file      : odb/oracle/container-statements.txx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset

namespace odb
{
  namespace oracle
  {
    // container_statements
    //
    template <typename T>
    container_statements<T>::
    container_statements (connection_type& conn, binding& id)
        : conn_ (conn),
          id_binding_ (id),
          functions_ (this),
          insert_image_binding_ (0, 0), // Initialized by impl.
          select_image_binding_ (0, 0), // Initialized by impl.
          svm_ (0)
    {
      functions_.insert_ = &traits::insert;
      functions_.select_ = &traits::select;
      functions_.delete__ = &traits::delete_;

      data_image_.version = 0;
      data_image_version_ = 0;
      data_id_binding_version_ = 0;
    }

    // smart_container_statements
    //
    template <typename T>
    smart_container_statements<T>::
    smart_container_statements (connection_type& conn, binding& id)
        : container_statements<T> (conn, id),
          cond_image_binding_ (0, 0),  // Initialized by impl.
          update_image_binding_ (0, 0) // Initialized by impl.
    {
      this->functions_.update_ = &traits::update;

      cond_image_.version = 0;
      cond_image_version_ = 0;
      cond_id_binding_version_ = 0;

      update_id_binding_version_ = 0;
      update_cond_image_version_ = 0;
      update_data_image_version_ = 0;
    }

    // container_statements_impl
    //
    template <typename T>
    container_statements_impl<T>::
    container_statements_impl (connection_type& conn, binding& id)
        : base (conn, id)
    {
      this->insert_image_binding_.bind = data_image_bind_;
      this->insert_image_binding_.count = traits::data_column_count;

      this->select_image_binding_.bind = data_image_bind_ +
        traits::id_column_count;
      this->select_image_binding_.count = traits::data_column_count -
        traits::id_column_count;

      std::memset (data_image_bind_, 0, sizeof (data_image_bind_));

      this->insert_text_ = traits::insert_statement;
      this->select_text_ = traits::select_statement;
      this->delete_text_ = traits::delete_statement;

      this->versioned_ = traits::versioned;
    }

    // smart_container_statements_impl
    //
    template <typename T>
    smart_container_statements_impl<T>::
    smart_container_statements_impl (connection_type& conn, binding& id)
        : container_statements_impl<T> (conn, id)
    {
      this->cond_image_binding_.bind = cond_image_bind_;
      this->cond_image_binding_.count = traits::cond_column_count;

      this->update_image_binding_.bind = update_image_bind_;
      this->update_image_binding_.count = traits::value_column_count +
        traits::cond_column_count;

      std::memset (cond_image_bind_, 0, sizeof (cond_image_bind_));
      std::memset (update_image_bind_, 0, sizeof (update_image_bind_));

      this->update_text_ = traits::update_statement;
    }
  }
}
