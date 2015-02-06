// file      : odb/qt/basic/oracle/qstring-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
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
    struct qstring_value_traits
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
    struct default_value_traits <QString, id_string>: qstring_value_traits
    {
    };

    template <>
    struct default_value_traits <QString, id_nstring>: qstring_value_traits
    {
    };

    class qstring_lob_value_traits
    {
    public:
      typedef QString value_type;
      typedef QString query_type;
      typedef lob_callback image_type;

      static void
      set_value (QString& v,
                 result_callback_type& cb,
                 void*& context,
                 bool is_null)
      {
        if (is_null)
          v = QString ();
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

      static bool
      result_callback (void* context,
                       ub4*,
                       void* b,
                       ub4 s,
                       chunk_position p)
      {
        QString& v (*static_cast<QString*> (context));

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
            v += QString::fromUtf8(static_cast<char*> (b),
                                   static_cast<int> (s));
            break;
          }
        }

        return true;
      }

      static bool
      param_callback (const void* context,
                      ub4* position_context,
                      const void** b,
                      ub4* s,
                      chunk_position* p,
                      void* temp_buffer,
                      ub4 capacity)
      {
        const QByteArray& v (static_cast<const QString*> (context)->toUtf8 ());

        *s = static_cast<ub4> (v.size ());

        if (*position_context == 0)
        {
          if (*s <= capacity)
            *p = chunk_one;
          else
          {
            *s = capacity;
            *p = chunk_first;
          }
        }
        else
        {
          *s -= *position_context;

          if (*s <= capacity)
            *p = chunk_last;
          else
          {
            *s = capacity;
            *p = chunk_next;
          }
        }

        std::memcpy (temp_buffer, v.constData () + *position_context, *s);
        *b = temp_buffer;
        *position_context += *s;

        return true;
      }
    };

    template <>
    struct default_value_traits<QString, id_clob>: qstring_lob_value_traits
    {
    };

    template <>
    struct default_value_traits<QString, id_nclob>: qstring_lob_value_traits
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

#endif // ODB_QT_BASIC_ORACLE_QSTRING_TRAITS_HXX
