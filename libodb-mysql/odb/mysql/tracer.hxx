// file      : odb/mysql/tracer.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_TRACER_HXX
#define ODB_MYSQL_TRACER_HXX

#include <odb/pre.hxx>

#include <odb/tracer.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class LIBODB_MYSQL_EXPORT tracer: private odb::tracer
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
      // Allow these classes to convert mysql::tracer to odb::tracer.
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

#endif // ODB_MYSQL_TRACER_HXX
