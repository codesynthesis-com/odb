// file      : odb/traversal/relational/key.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_KEY_HXX
#define ODB_TRAVERSAL_RELATIONAL_KEY_HXX

#include <odb/semantics/relational/key.hxx>
#include <odb/traversal/relational/elements.hxx>

namespace traversal
{
  namespace relational
  {
    struct contains: edge<semantics::relational::contains>
    {
      contains () {}
      contains (node_dispatcher& n) {node_traverser (n);}

      virtual void
      traverse (type&);
    };

    template <typename T>
    struct key_template: node<T>
    {
    public:
      virtual void
      traverse (T& k)
      {
        contains (k);
      }

      virtual void
      contains (T& k)
      {
        contains (k, *this);
      }

      virtual void
      contains (T& k, edge_dispatcher& d)
      {
        this->iterate_and_dispatch (k.contains_begin (), k.contains_end (), d);
      }
    };

    struct key: key_template<semantics::relational::key> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_KEY_HXX
