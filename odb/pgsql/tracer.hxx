// file      : odb/pgsql/tracer.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_TRACER_HXX
#define ODB_PGSQL_TRACER_HXX

#include <odb/pre.hxx>

#include <odb/tracer.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class LIBODB_PGSQL_EXPORT tracer: private odb::tracer
    {
    public:
      virtual
      ~tracer ();

      virtual void
      prepare (connection&, const statement&);

      virtual void
      execute (connection&, const statement&);

      virtual void
      execute (connection&, const char* statement) = 0;

      virtual void
      deallocate (connection&, const statement&);

    private:
      // Allow these classes to convert pgsql::tracer to odb::tracer.
      //
      friend class database;
      friend class connection;
      friend class transaction;

      virtual void
      prepare (odb::connection&, const odb::statement&);

      virtual void
      execute (odb::connection&, const odb::statement&);

      virtual void
      execute (odb::connection&, const char* statement);

      virtual void
      deallocate (odb::connection&, const odb::statement&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_TRACER_HXX
