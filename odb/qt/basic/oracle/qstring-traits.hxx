// file      : odb/qt/basic/oracle/qstring-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_ORACLE_QSTRING_TRAITS_HXX
#define ODB_QT_BASIC_ORACLE_QSTRING_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t

#include <QtCore/QString>

#include <odb/oracle/traits.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits <QString, id_string>
    {
    public:
      typedef QString value_type;
      typedef QString query_type;
      typedef char* image_type;

      static void
      set_value (QString& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = QString ();
        else
          v = QString::fromUtf8 (b, static_cast<int> (n));
      }

      static void
      set_image (char* b,
                 std::size_t c,
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

          if (n > c)
            n = c;

          std::memcpy (b, a.constData (), n);
        }
      }
    };

    template <>
    struct default_type_traits<QString>
    {
      static const database_type_id db_type_id = id_string;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_ORACLE_QSTRING_TRAITS_HXX
