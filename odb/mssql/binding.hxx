// file      : odb/mssql/binding.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
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
      typedef mssql::change_callback change_callback_type;

      binding ()
        : bind (0), count (0), version (0),
          batch (0), skip (0), status (0),
          change_callback (0) {}

      binding (bind_type* b, std::size_t n)
        : bind (b), count (n), version (0),
          batch (1), skip (0), status (0),
          change_callback (0)
      {
      }

      binding (bind_type* b, std::size_t n,
               std::size_t bt, std::size_t s, SQLUSMALLINT* st)
        : bind (b), count (n), version (0),
          batch (bt), skip (s), status (st),
          change_callback (0)
      {
      }

      bind_type* bind;
      std::size_t count;
      std::size_t version;

      std::size_t batch;
      std::size_t skip;
      SQLUSMALLINT* status; // Batch status array.

      change_callback_type* change_callback;

    private:
      binding (const binding&);
      binding& operator= (const binding&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_BINDING_HXX
