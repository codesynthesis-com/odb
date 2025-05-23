// file      : odb/oracle/tracer.cxx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/oracle/tracer.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/statement.hxx>

namespace odb
{
  namespace oracle
  {
    tracer::
    ~tracer ()
    {
    }

    void tracer::
    prepare (connection&, const statement&)
    {
    }

    void tracer::
    execute (connection& c, const statement& s)
    {
      execute (c, s.text ());
    }

    void tracer::
    deallocate (connection&, const statement&)
    {
    }

    void tracer::
    prepare (odb::connection& c, const odb::statement& s)
    {
      prepare (static_cast<connection&> (c),
               static_cast<const statement&> (s));
    }

    void tracer::
    execute (odb::connection& c, const odb::statement& s)
    {
      execute (static_cast<connection&> (c),
               static_cast<const statement&> (s));
    }

    void tracer::
    execute (odb::connection& c, const char* s)
    {
      execute (static_cast<connection&> (c), s);
    }

    void tracer::
    deallocate (odb::connection& c, const odb::statement& s)
    {
      deallocate (static_cast<connection&> (c),
                  static_cast<const statement&> (s));
    }
  }
}
