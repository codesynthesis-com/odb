// file      : odb/qt/basic/pgsql/uuid-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_PGSQL_UUID_TRAITS_HXX
#define ODB_QT_BASIC_PGSQL_UUID_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy

#include <QtCore/QUuid>

#include <odb/pgsql/traits.hxx>

namespace odb
{
  namespace pgsql
  {
    template <>
    class default_value_traits<QUuid, id_uuid>
    {
    public:
      typedef QUuid value_type;
      typedef value_type query_type;
      typedef unsigned char* image_type;

      // PostgreSQL binary UUID representation is big-endian in the RFC 4122,
      // section 4.1.2 order. While Qt provides (since 4.8) to/fromRfc4122(),
      // they both incur a memory allocation (by QByteArray) which we could
      // avoid, if we did it ourselves.
      //

      static void
      set_value (value_type& v, const unsigned char* i, bool is_null)
      {
        if (!is_null)
          v = QUuid::fromRfc4122 (
            QByteArray (reinterpret_cast<const char*> (i), 16));
        else
          v = QUuid ();
      }

      static void
      set_image (unsigned char* i, bool& is_null, const value_type& v)
      {
        // If we can, store nil as NULL. Otherwise, store it as a value.
        //
        is_null = is_null && v.isNull ();

        if (!is_null)
          std::memcpy (i, v.toRfc4122 ().constData (), 16);
      }
    };

    template <>
    struct default_type_traits<QUuid>
    {
      static const database_type_id db_type_id = id_uuid;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_PGSQL_UUID_TRAITS_HXX
