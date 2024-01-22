// file      : odb/traversal/union.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_UNION_HXX
#define ODB_TRAVERSAL_UNION_HXX

#include <odb/semantics/union.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct union_: scope_template<semantics::union_> {};
}

#endif // ODB_TRAVERSAL_UNION_HXX
