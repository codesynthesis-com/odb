// file      : odb/qt/basic/oracle/qbyte-array-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
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

          if (n > c)
            n = c;

          std::memcpy (b, v.constData (), n);
        }
      }
    };

    template <>
    struct default_value_traits<QByteArray, id_blob>
    {
      typedef QByteArray value_type;
      typedef QByteArray query_type;
      typedef lob_callback image_type;

      static void
      set_value (QByteArray& v,
                 result_callback_type& cb,
                 void*& context,
                 bool is_null)
      {
        if (is_null)
          v = QByteArray ();
        else
        {
          cb = &result_callback;
          context = &v;
        }
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

      static bool
      result_callback (void* context,
                       ub4*,
                       void* b,
                       ub4 s,
                       chunk_position p)
      {
        QByteArray& v (*static_cast<QByteArray*> (context));

        switch (p)
        {
        case chunk_one:
        case chunk_first:
          {
            v.clear ();

            // Falling through.
          }
        case chunk_next:
        case chunk_last:
          {
            v.append (static_cast<char*> (b), static_cast<int> (s));
            break;
          }
        }

        return true;
      }

      static bool
      param_callback (const void* context,
                      ub4*,
                      const void** b,
                      ub4* s,
                      chunk_position* p,
                      void*,
                      ub4)
      {
        const QByteArray& v (*static_cast<const QByteArray*> (context));

        *p = chunk_one;
        *s = static_cast<ub4> (v.size ());
        *b = v.constData ();

        return true;
      }
    };

    template <>
    struct default_type_traits<QByteArray>
    {
      // Allow use of QByteArray in query expressions by default by specifying
      // the default type id as RAW.
      //
      static const database_type_id db_type_id = id_raw;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_ORACLE_QBYTE_ARRAY_TRAITS_HXX
