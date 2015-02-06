// file      : odb/mssql/tracer.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_TRACER_HXX
#define ODB_MSSQL_TRACER_HXX

#include <odb/pre.hxx>

#include <odb/tracer.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class LIBODB_MSSQL_EXPORT tracer: private odb::tracer
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
      // Allow these classes to convert mssql::tracer to odb::tracer.
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

#endif // ODB_MSSQL_TRACER_HXX
