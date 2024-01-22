// file      : odb/traversal/relational/primary-key.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_PRIMARY_KEY_HXX
#define ODB_TRAVERSAL_RELATIONAL_PRIMARY_KEY_HXX

#include <odb/semantics/relational/primary-key.hxx>
#include <odb/traversal/relational/key.hxx>

namespace traversal
{
  namespace relational
  {
    struct primary_key: key_template<semantics::relational::primary_key> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_PRIMARY_KEY_HXX
