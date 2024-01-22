// file      : odb/relational/inline.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/inline.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

namespace relational
{
  namespace inline_
  {
    void
    generate ()
    {
      context ctx;
      ostream& os (ctx.os);

      instance<include> i;
      i->generate ();

      traversal::unit unit;
      traversal::defines unit_defines;
      typedefs unit_typedefs (false);
      traversal::namespace_ ns;
      class_ c;

      unit >> unit_defines >> ns;
      unit_defines >> c;
      unit >> unit_typedefs >> c;

      traversal::defines ns_defines;
      typedefs ns_typedefs (false);

      ns >> ns_defines >> ns;
      ns_defines >> c;
      ns >> ns_typedefs >> c;

      os << "namespace odb"
         << "{";

      unit.dispatch (ctx.unit);

      os << "}";
    }
  }
}
