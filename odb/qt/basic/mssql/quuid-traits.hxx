// file      : odb/qt/basic/mssql/uuid-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MSSQL_UUID_TRAITS_HXX
#define ODB_QT_BASIC_MSSQL_UUID_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy

#include <QtCore/QUuid>

#include <odb/mssql/traits.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    class default_value_traits<QUuid, id_uniqueidentifier>
    {
    public:
      typedef QUuid value_type;
      typedef value_type query_type;
      typedef uniqueidentifier image_type;

      static void
      set_value (value_type& v, const uniqueidentifier& i, bool is_null)
      {
        if (!is_null)
          std::memcpy (&v.data1, &i, 16);
        else
          v = QUuid ();
      }

      static void
      set_image (uniqueidentifier& i, bool& is_null, const value_type& v)
      {
        // If we can, store nil as NULL. Otherwise, store it as a value.
        //
        is_null = is_null && v.isNull ();

        if (!is_null)
          std::memcpy (&i, &v.data1, 16);
      }
    };

    template <>
    struct default_type_traits<QUuid>
    {
      static const database_type_id db_type_id = id_uniqueidentifier;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_MSSQL_UUID_TRAITS_HXX
