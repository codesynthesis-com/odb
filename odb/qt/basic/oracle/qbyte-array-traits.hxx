// file      : odb/qt/basic/oracle/qbyte-array-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_ORACLE_QBYTE_ARRAY_TRAITS_HXX
#define ODB_QT_BASIC_ORACLE_QBYTE_ARRAY_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t

#include <QtCore/QByteArray>

#include <odb/oracle/traits.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    struct default_value_traits<QByteArray, id_raw>
    {
      typedef QByteArray value_type;
      typedef QByteArray query_type;
      typedef char* image_type;

      static void
      set_value (QByteArray& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = QByteArray ();
        else
        {
          // Note that we cannot use replace() here since a suitable
          // overload was only added in Qt 4.7.
          //
          v.resize (static_cast<int> (n));
          std::memcpy (v.data (), b, n);
        }
      }

      static void
      set_image (char* b,
                 std::size_t c,
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

          //@@ Assert or throw?
          //
          assert (n <= c);

          std::memcpy (b, v.data (), n);
        }
      }
    };

    template <>
    struct default_type_traits<QByteArray>
    {
      static const database_type_id db_type_id = id_raw;
    };
  }
}

#endif // ODB_QT_BASIC_ORACLE_QBYTE_ARRAY_TRAITS_HXX
