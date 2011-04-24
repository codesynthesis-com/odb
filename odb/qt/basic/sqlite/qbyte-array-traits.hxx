// file      : odb/qt/basic/sqlite/qbyte-array-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_SQLITE_QBYTE_ARRAY_TRAITS_HXX
#define ODB_QT_BASIC_SQLITE_QBYTE_ARRAY_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t

#include <QByteArray>

#include <odb/details/buffer.hxx>
#include <odb/sqlite/traits.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    class default_value_traits<QByteArray, id_blob>
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
          v.replace (0, v.size (), b.data (), static_cast<int> (n));
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const QByteArray& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          n = static_cast<std::size_t> (v.size ());
          if (n > b.capacity ())
            b.capacity (n);

          std::memcpy (b.data (), v.data (), n);
        }
      }
    };

    template <>
    class default_type_traits<QByteArray>
    {
      static const database_type_id db_type_id = id_blob;
    };
  }
}

#endif // ODB_QT_BASIC_SQLITE_QBYTE_ARRAY_TRAITS_HXX
