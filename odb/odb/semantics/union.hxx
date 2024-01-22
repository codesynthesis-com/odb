// file      : odb/semantics/union.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_UNION_HXX
#define ODB_SEMANTICS_UNION_HXX

#include <odb/semantics/elements.hxx>

namespace semantics
{
  class union_: public virtual type, public scope
  {
  public:
    union_ (path const&, size_t line, size_t column, tree);

    // Resolve conflict between scope::scope and nameable::scope.
    //
    using nameable::scope;

  protected:
    union_ ()
    {
    }
  };
}

#endif // ODB_SEMANTICS_UNION_HXX
