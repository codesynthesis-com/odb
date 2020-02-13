// file      : odb/pgsql/container-statements.txx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset

namespace odb
{
  namespace pgsql
  {
    // container_statements
    //
    template <typename T>
    container_statements<T>::
    container_statements (connection_type& conn,
                          binding& id,
                          native_binding& idn,
                          const Oid* idt)
        : conn_ (conn),
          id_binding_ (id),
          id_native_binding_ (idn),
          id_types_ (idt),
          functions_ (this),
          insert_image_binding_ (0, 0),              // Initialized by impl.
          insert_image_native_binding_ (0, 0, 0, 0), // Initialized by impl.
          select_image_binding_ (0, 0),              // Initialized by impl.
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
    smart_container_statements (connection_type& conn,
                                binding& id,
                                native_binding& idn,
                                const Oid* idt)
        : container_statements<T> (conn, id, idn, idt),
          cond_image_binding_ (0, 0),                 // Initialized by impl.
          cond_image_native_binding_ (0, 0, 0, 0),    // Initialized by impl.
          update_image_binding_ (0, 0),               // Initialized by impl.
          update_image_native_binding_ (0, 0, 0, 0)   // Initialized by impl.
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
    container_statements_impl (connection_type& conn,
                               binding& id,
                               native_binding& idn,
                               const Oid* idt)
        : base (conn, id, idn, idt)
    {
      this->select_image_truncated_ = select_image_truncated_array_;

      this->insert_image_binding_.bind = data_image_bind_;
      this->insert_image_binding_.count = traits::data_column_count;

      this->select_image_binding_.bind = data_image_bind_ +
        traits::id_column_count;
      this->select_image_binding_.count = traits::data_column_count -
        traits::id_column_count;

      this->insert_image_native_binding_.values = data_image_values_;
      this->insert_image_native_binding_.lengths = data_image_lengths_;
      this->insert_image_native_binding_.formats = data_image_formats_;
      this->insert_image_native_binding_.count = traits::data_column_count;

      std::memset (data_image_bind_, 0, sizeof (data_image_bind_));
      std::memset (select_image_truncated_array_,
                   0,
                   sizeof (select_image_truncated_array_));

      for (std::size_t i (0);
           i < traits::data_column_count - traits::id_column_count;
           ++i)
        data_image_bind_[i + traits::id_column_count].truncated =
          select_image_truncated_array_ + i;

      this->insert_name_ = traits::insert_name;
      this->insert_text_ = traits::insert_statement;
      this->insert_types_ = traits::insert_types;
      this->insert_count_ = traits::data_column_count;

      this->select_name_ = traits::select_name;
      this->select_text_ = traits::select_statement;

      this->delete_name_ = traits::delete_name;
      this->delete_text_ = traits::delete_statement;

      this->versioned_ = traits::versioned;
    }

    // smart_container_statements_impl
    //
    template <typename T>
    smart_container_statements_impl<T>::
    smart_container_statements_impl (connection_type& conn,
                                     binding& id,
                                     native_binding& idn,
                                     const Oid* idt)
        : container_statements_impl<T> (conn, id, idn, idt)
    {
      this->cond_image_binding_.bind = cond_image_bind_;
      this->cond_image_binding_.count = traits::cond_column_count;

      this->update_image_binding_.bind = update_image_bind_;
      this->update_image_binding_.count = traits::value_column_count +
        traits::cond_column_count;

      this->cond_image_native_binding_.values = cond_image_values_;
      this->cond_image_native_binding_.lengths = cond_image_lengths_;
      this->cond_image_native_binding_.formats = cond_image_formats_;
      this->cond_image_native_binding_.count = traits::cond_column_count;

      this->update_image_native_binding_.values = update_image_values_;
      this->update_image_native_binding_.lengths = update_image_lengths_;
      this->update_image_native_binding_.formats = update_image_formats_;
      this->update_image_native_binding_.count = traits::value_column_count +
        traits::cond_column_count;

      std::memset (cond_image_bind_, 0, sizeof (cond_image_bind_));
      std::memset (update_image_bind_, 0, sizeof (update_image_bind_));

      this->update_name_ = traits::update_name;
      this->update_text_ = traits::update_statement;
      this->update_types_ = traits::update_types;
      this->update_count_ = traits::value_column_count +
        traits::cond_column_count;

      this->delete_types_ = traits::delete_types;
      this->delete_count_ = traits::cond_column_count;
    }
  }
}
