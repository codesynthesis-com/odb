// file      : odb/mysql/tracer.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/mysql/tracer.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/statement.hxx>

namespace odb
{
  namespace mysql
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
