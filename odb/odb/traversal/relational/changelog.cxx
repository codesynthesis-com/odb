// file      : odb/traversal/relational/changelog.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/relational/changelog.hxx>
#include <odb/traversal/relational/model.hxx>
#include <odb/traversal/relational/changeset.hxx>

namespace traversal
{
  namespace relational
  {
    // contains_model
    //
    void contains_model::
    traverse (type& c)
    {
      dispatch (c.model ());
    }

    // contains_changeset
    //
    void contains_changeset::
    traverse (type& c)
    {
      dispatch (c.changeset ());
    }

    // changelog
    //
    void changelog::
    traverse (type& cl)
    {
      contains_model (cl);
      contains_changeset (cl);
    }

    void changelog::
    contains_model (type& cl)
    {
      contains_model (cl, *this);
    }

    void changelog::
    contains_model (type& cl, edge_dispatcher& d)
    {
      d.dispatch (cl.contains_model ());
    }

    void changelog::
    contains_changeset (type& cl)
    {
      contains_changeset (cl, *this);
    }

    void changelog::
    contains_changeset (type& cl, edge_dispatcher& d)
    {
      iterate_and_dispatch (cl.contains_changeset_begin (),
                            cl.contains_changeset_end (),
                            d);
    }
  }
}
