// file      : odb/mssql/no-id-object-statements.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::memset

namespace odb
{
  namespace mssql
  {
    template <typename T>
    no_id_object_statements<T>::
    ~no_id_object_statements ()
    {
    }

    template <typename T>
    no_id_object_statements<T>::
    no_id_object_statements (connection_type& conn)
        : statements_base (conn),
          select_image_binding_ (select_image_bind_, select_column_count),
          insert_image_binding_ (insert_image_bind_,
                                 insert_column_count,
                                 object_traits::batch,
                                 sizeof (image_type),
                                 status_)
    {
      image_[0].version = 0; // Only version in the first element used.
      select_image_version_ = 0;
      insert_image_version_ = 0;

      // SELECT statements only use the first element (no batches).
      //
      select_image_binding_.change_callback = image_[0].change_callback ();

      std::memset (insert_image_bind_, 0, sizeof (insert_image_bind_));
      std::memset (select_image_bind_, 0, sizeof (select_image_bind_));
    }
  }
}
