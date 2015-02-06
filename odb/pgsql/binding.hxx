// file      : odb/pgsql/binding.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
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
      native_binding (char** v,
                      int* l,
                      int* f,
                      std::size_t n)
        : values (v), lengths (l), formats (f), count (n)
      {
      }

      char** values;
      int* lengths;
      int* formats;
      std::size_t count;

    private:
      native_binding (const native_binding&);
      native_binding& operator= (const native_binding&);
    };

    class LIBODB_PGSQL_EXPORT binding
    {
    public:
      typedef pgsql::bind bind_type;

      binding (): bind (0), count (0), version (0) {}

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
