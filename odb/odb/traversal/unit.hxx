// file      : odb/traversal/unit.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_UNIT_HXX
#define ODB_TRAVERSAL_UNIT_HXX

#include <odb/semantics/unit.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct unit: scope_template<semantics::unit> {};
}

#endif // ODB_TRAVERSAL_UNIT_HXX
