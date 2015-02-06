// file      : odb/mssql/section-statements.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::memset

namespace odb
{
  namespace mssql
  {
    template <typename T, typename ST>
    section_statements<T, ST>::
    section_statements (connection_type& conn,
                        image_type& im, id_image_type& idim,
                        binding& id, binding& idv)
        : conn_ (conn),
          svm_ (0),
          image_ (im),
          id_image_ (idim),
          id_binding_ (id),
          idv_binding_ (idv),
          select_image_binding_ (select_image_bind_,
                                 select_column_count +
                                 managed_optimistic_load_column_count),
          update_image_binding_ (update_image_bind_,
                                 update_column_count + id_column_count +
                                 managed_optimistic_update_column_count)
    {
      select_image_version_ = 0;
      update_image_version_ = 0;
      update_id_binding_version_ = 0;

      // If we are using optimistic concurrency, then the select statement
      // will override the version member in the object image.
      //
      if (managed_optimistic_load_column_count != 0)
      {
        // Getting to the root image in polymorphic case is tricky.
        //
        typedef object_traits_impl<T, id_mssql> object_traits;

        select_image_binding_.change_callback =
          root_image<object_traits, object_traits::polymorphic>::get (
            image_).change_callback ();
      }

      std::memset (select_image_bind_, 0, sizeof (select_image_bind_));
      std::memset (update_image_bind_, 0, sizeof (update_image_bind_));
    }
  }
}
