// file      : odb/traversal/relational/key.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/relational/key.hxx>
#include <odb/traversal/relational/column.hxx>

namespace traversal
{
  namespace relational
  {
    void contains::
    traverse (type& c)
    {
      dispatch (c.column ());
    }
  }
}
