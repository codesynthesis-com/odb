// file      : odb/qt/basic/mssql/qbyte-array-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MSSQL_QBYTE_ARRAY_TRAITS_HXX
#define ODB_QT_BASIC_MSSQL_QBYTE_ARRAY_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t
#include <cassert>

#include <QtCore/QByteArray>

#include <odb/mssql/traits.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    struct default_value_traits<QByteArray, id_binary>
    {
      typedef QByteArray value_type;
      typedef QByteArray query_type;
      typedef char* image_type;

      static void
      set_value (QByteArray& v, const char* b, std::size_t n, bool is_null)
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

          if (n > c)
            n = c;

          std::memcpy (b, v.constData (), n);
        }
      }
    };

    template <>
    struct default_value_traits<QByteArray, id_long_binary>
    {
      typedef QByteArray value_type;
      typedef QByteArray query_type;
      typedef long_callback image_type;

      static void
      set_value (QByteArray& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = &v;
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const QByteArray& v)
      {
        if (v.isNull ())
          is_null = true;
        else
        {
          is_null = false;
          cb = &param_callback;
          context = &v;
        }
      }

      static void
      param_callback (const void* context,
                      std::size_t*,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void*,
                      std::size_t)
      {
        const QByteArray& v (*static_cast<const QByteArray*> (context));

        *buffer = v.constData ();
        *size = static_cast<std::size_t> (v.size ());
        *chunk = chunk_one;
      }

      static void
      result_callback (void* context,
                       std::size_t*,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void*,
                       std::size_t)
      {
        QByteArray& v (*static_cast<QByteArray*> (context));

        switch (chunk)
        {
        case chunk_null:
          {
            v = QByteArray ();
            break;
          }
        case chunk_one:
          {
            v.clear ();
            break;
          }
        case chunk_first:
          {
            // The Native Client ODBC driver seems to always be able to
            // return the total size. This makes things simple and
            // efficient.
            //
            assert (size_left != 0);

            v.resize (static_cast<int> (size_left));
            *buffer = v.data ();
            *size = size_left;
            break;
          }
        case chunk_next:
          {
            // We should never get here.
            //
            assert (false);
            break;
          }
        case chunk_last:
          {
            // Nothing to do here. The array is already of the correct size
            // and should contain the data.
            break;
          }
        }
      }
    };

    template <>
    struct default_type_traits<QByteArray>
    {
      static const database_type_id db_type_id = id_long_binary;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_MSSQL_QBYTE_ARRAY_TRAITS_HXX
