// file      : odb/mysql/enums.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_ENUMS_HXX
#define ODB_MYSQL_ENUMS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t
#include <cstring> // std::memmove
#include <cassert>

#include <odb/details/buffer.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/traits.hxx>
#include <odb/mysql/version.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    // Common interface for working with the dual enum image (integer or
    // string). Used by the generated code and query machinery.
    //
    // We use overload resolution instead of template specialization to
    // avoid having to specify the image type explicitly.
    //
    struct LIBODB_MYSQL_EXPORT enum_traits
    {
      //
      // Integer image.
      //
      static void
      bind (MYSQL_BIND& b, unsigned short& i, unsigned long&, my_bool* is_null)
      {
        b.buffer_type = MYSQL_TYPE_SHORT;
        b.is_unsigned = 1;
        b.buffer = &i;
        b.is_null = is_null;
      }

      static bool
      grow (unsigned short&, unsigned long&)
      {
        return false;
      }

      template <typename T>
      static bool
      set_image (unsigned short& i, unsigned long&, bool& is_null, const T& v)
      {
        value_traits<T, id_enum>::set_image (i, is_null, v);
        return false;
      }

      template <typename T>
      static void
      set_value (T& v, unsigned short i, unsigned long, bool is_null)
      {
        value_traits<T, id_enum>::set_value (v, i, is_null);
      }

      //
      // String image.
      //

      static void
      bind (MYSQL_BIND& b,
            details::buffer& i,
            unsigned long& size,
            my_bool* is_null)
      {
        b.buffer_type = MYSQL_TYPE_STRING;
        b.buffer = i.data ();
        b.buffer_length = static_cast<unsigned long> (i.capacity ());
        b.length = &size;
        b.is_null = is_null;
      }

      static bool
      grow (details::buffer& i, unsigned long& size)
      {
        i.capacity (size);
        return true;
      }

      template <typename T>
      static bool
      set_image (details::buffer& i,
                 unsigned long& size,
                 bool& is_null,
                 const T& v)
      {
        std::size_t n (0), c (i.capacity ());
        value_traits<T, id_enum>::set_image (i, n, is_null, v);
        size = static_cast<unsigned long> (n);
        return c != i.capacity ();
      }

      template <typename T>
      static void
      set_value (T& v,
                 const details::buffer& i,
                 unsigned long size,
                 bool is_null)
      {
        // The buffer has the following content: "<num> <str>". Get rid of
        // the numeric value so that we have "<str>". For more information
        // why it is done this way, see mysql::object_columns class in the
        // ODB compiler.
        //
        strip_value (i, size);

        value_traits<T, id_enum>::set_value (v, i, size, is_null);
      }

    private:
      static void
      strip_value (const details::buffer& i, unsigned long& size);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_ENUMS_HXX
