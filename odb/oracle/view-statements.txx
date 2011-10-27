// file      : odb/oracle/view-statements.txx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset

namespace odb
{
  namespace oracle
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

      image_binding_.change_callback = &image_.change_callback;

      std::memset (image_bind_, 0, sizeof (image_bind_));
      std::memset (image_indicator_, 0, sizeof (image_indicator_));

      for (std::size_t i (0); i < view_traits::column_count; ++i)
        image_bind_[i].indicator = image_indicator_ + i;
    }
  }
}
