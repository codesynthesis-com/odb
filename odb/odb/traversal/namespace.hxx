// file      : odb/traversal/namespace.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_NAMESPACE_HXX
#define ODB_TRAVERSAL_NAMESPACE_HXX

#include <odb/semantics/namespace.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct namespace_: scope_template<semantics::namespace_> {};
}

#endif // ODB_TRAVERSAL_NAMESPACE_HXX
