// file      : odb/qt/basic/mysql/uuid-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MYSQL_UUID_TRAITS_HXX
#define ODB_QT_BASIC_MYSQL_UUID_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cassert>

#include <QtCore/QUuid>

#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    struct default_value_traits<QUuid, id_blob>
    {
      typedef QUuid value_type;
      typedef value_type query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
        {
          assert (n == 16);
          std::memcpy (&v.data1, b.data (), 16);
        }
        else
          v = QUuid ();
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        // If we can, store nil as NULL. Otherwise, store it as a value.
        //
        is_null = is_null && v.isNull ();

        if (!is_null)
        {
          n = 16;

          if (n > b.capacity ())
            b.capacity (n);

          std::memcpy (b.data (), &v.data1, n);
        }
      }
    };

    template <>
    struct default_type_traits<QUuid>
    {
      static const database_type_id db_type_id = id_blob;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_MYSQL_UUID_TRAITS_HXX
