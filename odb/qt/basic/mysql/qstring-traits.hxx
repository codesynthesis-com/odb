// file      : odb/qt/basic/mysql/qstring-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MYSQL_QSTRING_TRAITS_HXX
#define ODB_QT_BASIC_MYSQL_QSTRING_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t

#include <QtCore/QString>

#include <odb/details/buffer.hxx>
#include <odb/mysql/traits.hxx>

namespace odb
{
  namespace mysql
  {
    class qstring_value_traits
    {
    public:
      typedef QString value_type;
      typedef QString query_type;
      typedef details::buffer image_type;

      static void
      set_value (QString& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = QString ();
        else
          v = QString::fromUtf8 (b.data (), static_cast<int> (n));
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const QString& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;

          const QByteArray& a (v.toUtf8 ());
          n = static_cast<std::size_t> (a.size ());

          if (n > b.capacity ())
            b.capacity (n);

          std::memcpy (b.data (), a.data (), n);
        }
      }
    };

    template <>
    struct default_value_traits<QString, id_string>: qstring_value_traits
    {
    };

    template <>
    struct default_value_traits<QString, id_decimal>: qstring_value_traits
    {
    };

    template <>
    struct default_value_traits<QString, id_enum>: qstring_value_traits
    {
    };

    template <>
    struct default_value_traits<QString, id_set>: qstring_value_traits
    {
    };

    template <>
    struct default_type_traits<QString>
    {
      static const database_type_id db_type_id = id_string;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_MYSQL_QSTRING_TRAITS_HXX
