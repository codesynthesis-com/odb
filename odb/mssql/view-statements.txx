// file      : odb/mssql/view-statements.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::memset

namespace odb
{
  namespace mssql
  {
    template <typename T>
    view_statements<T>::
    ~view_statements ()
    {
    }

    template <typename T>
    view_statements<T>::
    view_statements (connection_type& conn)
        : statements_base (conn),
          image_binding_ (image_bind_, view_traits::column_count)
    {
      image_.version = 0;
      image_version_ = 0;

      image_binding_.change_callback = image_.change_callback ();

      std::memset (image_bind_, 0, sizeof (image_bind_));
    }
  }
}
