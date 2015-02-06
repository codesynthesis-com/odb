// file      : odb/qt/basic/mssql/qstring-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MSSQL_QSTRING_TRAITS_HXX
#define ODB_QT_BASIC_MSSQL_QSTRING_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstring> // std::memcpy
#include <cstddef> // std::size_t
#include <cassert>

#include <QtCore/QString>

#include <odb/mssql/traits.hxx>

namespace odb
{
  namespace mssql
  {
    template <>
    struct default_value_traits<QString, id_string>
    {
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
          // On Windows the string data is in Windows code page. On Linux
          // it is always UTF-8.
          //
#ifdef _WIN32
          v = QString::fromLocal8Bit (b, static_cast<int> (n));
#else
          v = QString::fromUtf8 (b, static_cast<int> (n));
#endif
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

#ifdef _WIN32
          const QByteArray& a (v.toLocal8Bit ());
#else
          const QByteArray& a (v.toUtf8 ());
#endif
          n = static_cast<std::size_t> (a.size ());

          if (n > c)
            n = c;

          std::memcpy (b, a.constData (), n);
        }
      }
    };

    template <>
    struct default_value_traits<QString, id_nstring>
    {
      typedef QString value_type;
      typedef QString query_type;
      typedef ucs2_char* image_type;

      static void
      set_value (QString& v,
                 const ucs2_char* b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = QString ();
        else
        {
          // Note that we cannot use replace() here since a suitable
          // overload was only added in Qt 4.7.
          //
          v.resize (static_cast<int> (n));
          std::memcpy (v.data (), b, n * 2);
        }
      }

      static void
      set_image (ucs2_char* b,
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
          n = static_cast<std::size_t> (v.size ());

          if (n > c)
            n = c;

          std::memcpy (b, v.constData (), n * 2);
        }
      }
    };

    template <>
    struct default_value_traits<QString, id_long_string>
    {
      typedef QString value_type;
      typedef QString query_type;
      typedef long_callback image_type;

      static void
      set_value (QString& v,
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
                 const QString& v)
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
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buf,
                      std::size_t tmp_capacity)
      {
        const QString& s (*static_cast<const QString*> (context));

#ifdef _WIN32
        const QByteArray& v (s.toLocal8Bit ());
#else
        const QByteArray& v (s.toUtf8 ());
#endif

        *size = static_cast<std::size_t> (v.size ());

        if (*position == 0)
        {
          if (*size <= tmp_capacity)
            *chunk = chunk_one;
          else
          {
            *size = tmp_capacity;
            *chunk = chunk_first;
          }
        }
        else
        {
          *size -= *position;

          if (*size <= tmp_capacity)
            *chunk = chunk_last;
          else
          {
            *size = tmp_capacity;
            *chunk = chunk_next;
          }
        }

        //@@ We might split a multi-byte sequence. Microsoft ODBC driver
        //   doesn't support this.
        //
        std::memcpy (tmp_buf, v.constData () + *position, *size);
        *buffer = tmp_buf;
        *position += *size;
      }

      static void
      result_callback (void* context,
                       std::size_t*,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t,
                       void* tmp_buf,
                       std::size_t tmp_capacity)
      {
        QString& v (*static_cast<QString*> (context));

        switch (chunk)
        {
        case chunk_null:
          {
            v = QString ();
            break;
          }
        case chunk_one:
          {
            v.clear ();
            break;
          }
        case chunk_first:
          {
            break;
          }
        case chunk_next:
        case chunk_last:
          {
            // Append the data from the temporary buffer.
            //
#ifdef _WIN32
            v += QString::fromLocal8Bit (static_cast<char*> (tmp_buf),
                                         static_cast<int> (*size));
#else
            v += QString::fromUtf8 (static_cast<char*> (tmp_buf),
                                    static_cast<int> (*size));
#endif
            break;
          }
        }

        if (chunk == chunk_first || chunk == chunk_next)
        {
          *buffer = tmp_buf;
          *size = tmp_capacity;
        }
      }
    };

    template <>
    struct default_value_traits<QString, id_long_nstring>
    {
      typedef QString value_type;
      typedef QString query_type;
      typedef long_callback image_type;

      static void
      set_value (QString& v,
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
                 const QString& v)
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
        const QString& v (*static_cast<const QString*> (context));

        *buffer = v.constData ();
        *size = static_cast<std::size_t> (v.size ()) * 2;
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
        QString& v (*static_cast<QString*> (context));

        switch (chunk)
        {
        case chunk_null:
          {
            v = QString ();
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

            size_left /= 2; // Convert to characters.
            size_left++;    // One extra for the null terminator.

            v.resize (static_cast<int> (size_left));
            *buffer = v.data ();
            *size = size_left * 2; // In bytes.
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
            // Get rid of the null terminator.
            //
            v.resize (static_cast<int> (*size / 2));
            break;
          }
        }
      }
    };

    template <>
    struct default_type_traits<QString>
    {
      static const database_type_id db_type_id = id_long_string;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_BASIC_MSSQL_QSTRING_TRAITS_HXX
