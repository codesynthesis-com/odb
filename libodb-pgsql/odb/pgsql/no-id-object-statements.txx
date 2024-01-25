// file      : odb/pgsql/no-id-object-statements.txx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstring> // std::memset

namespace odb
{
  namespace pgsql
  {
    //
    // no_id_object_statements
    //

    template <typename T>
    no_id_object_statements<T>::
    ~no_id_object_statements ()
    {
    }

    template <typename T>
    no_id_object_statements<T>::
    no_id_object_statements (connection_type& conn)
        : statements_base (conn),
          // select
          select_image_binding_ (select_image_bind_, select_column_count),
          // insert
          insert_image_binding_ (insert_image_bind_,
                                 insert_column_count,
                                 object_traits::batch,
                                 sizeof (image_type),
                                 status_),
          insert_image_native_binding_ (insert_image_values_,
                                        insert_image_lengths_,
                                        insert_image_formats_,
                                        insert_column_count)
    {
      image_[0].version = 0; // Only version in the first element used.
      select_image_version_ = 0;
      insert_image_version_ = 0;

      std::memset (insert_image_bind_, 0, sizeof (insert_image_bind_));
      std::memset (select_image_bind_, 0, sizeof (select_image_bind_));
      std::memset (
        select_image_truncated_, 0, sizeof (select_image_truncated_));

      for (std::size_t i (0); i < select_column_count; ++i)
        select_image_bind_[i].truncated = select_image_truncated_ + i;
    }
  }
}
