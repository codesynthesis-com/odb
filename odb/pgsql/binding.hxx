// file      : odb/pgsql/binding.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_BINDING_HXX
#define ODB_PGSQL_BINDING_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/pgsql-types.hxx>

namespace odb
{
  namespace pgsql
  {
    class native_binding
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

    class binding
    {
    public:
      typedef pgsql::bind bind_type;

      binding ()
          : bind (0), count (0), version (0),
            batch (0), skip (0), status (0) {}

      binding (bind_type* b, std::size_t n)
          : bind (b), count (n), version (0),
            batch (0), skip (0), status (0) {}

      binding (bind_type* b, std::size_t n,
               std::size_t bt, std::size_t s, unsigned long long* st)
          : bind (b), count (n), version (0),
            batch (bt), skip (s), status (st) {}

      bind_type* bind;
      std::size_t count;
      std::size_t version;

      std::size_t batch;
      std::size_t skip;
      unsigned long long* status; // Batch status array.

    private:
      binding (const binding&);
      binding& operator= (const binding&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_BINDING_HXX
