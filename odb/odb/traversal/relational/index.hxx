// file      : odb/traversal/relational/index.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_INDEX_HXX
#define ODB_TRAVERSAL_RELATIONAL_INDEX_HXX

#include <odb/semantics/relational/index.hxx>
#include <odb/traversal/relational/key.hxx>

namespace traversal
{
  namespace relational
  {
    struct index: key_template<semantics::relational::index> {};
    struct add_index: key_template<semantics::relational::add_index> {};
    struct drop_index: node<semantics::relational::drop_index> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_INDEX_HXX
