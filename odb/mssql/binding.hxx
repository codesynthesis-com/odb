// file      : odb/mssql/binding.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_BINDING_HXX
#define ODB_MSSQL_BINDING_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/mssql/version.hxx>
#include <odb/mssql/mssql-types.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class LIBODB_MSSQL_EXPORT binding
    {
    public:
      typedef mssql::bind bind_type;

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

#endif // ODB_MSSQL_BINDING_HXX
