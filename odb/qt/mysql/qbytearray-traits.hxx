// file      : odb/qt/mysql/qbytearray-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_MYSQL_QBYTEARRAY_TRAITS_HXX
#define ODB_QT_MYSQL_QBYTEARRAY_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t

#include <QByteArray>

#include <odb/details/buffer.hxx>
#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    class default_value_traits<QByteArray, details::buffer, id_blob>
    {
    public:
      typedef QByteArray value_type;
      typedef QByteArray query_type;
      typedef details::buffer image_type;

      static void
      set_value (QByteArray& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = QByteArray ();
        else
        {
          if (v.capacity () < n + 1)
            v.reserve (n + 1);

          std::memcpy (v.data (), b.data (), n);
          v.resize (n);
        }
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const QByteArray& v)
      {
        if (v.is_null)
          is_null = true;
        else
        {
          n = v.size ();
          if (n > b.capacity ())
            b.capacity (n);

          if (n != 0)
            std::memcpy (v.data (), b.data (), n);
        }
      }
    };
  }
}

#endif // ODB_QT_MYSQL_QBYTEARRAY_TRAITS_HXX
