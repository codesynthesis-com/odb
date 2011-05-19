// file      : odb/pgsql/binding.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_BINDING_HXX
#define ODB_PGSQL_BINDING_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/pgsql-types.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class LIBODB_PGSQL_EXPORT native_binding
    {
    public:

      native_binding (const char* const* v,
                      const int* l,
                      const int* f,
                      std::size_t n)
        : values (v), lengths (l), formats (f), version (0), count (n)
      {
      }

      const char* const* values;
      const int* lengths;
      const int* formats;
      std::size_t version;
      std::size_t count;

    private:
      native_binding (const native_binding&);
      native_binding& operator= (const native_binding&);
    };

    class LIBODB_PGSQL_EXPORT binding
    {
    public:
      typedef pgsql::bind bind_type;

      binding (bind_type* b, std::size_t n)
        : bind (b), count (n), version (0)
      {
      }

      bind_type* bind;
      std::size_t count;
      std::size_t version;

    private:
      binding (const binding&);
      binding& operator= (const binding&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_BINDING_HXX
