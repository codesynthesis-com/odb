// file      : odb/traversal/elements.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_ELEMENTS_HXX
#define ODB_TRAVERSAL_ELEMENTS_HXX

#include <libcutl/compiler/traversal.hxx>
#include <odb/semantics/elements.hxx>

namespace traversal
{
  using namespace cutl;

  //
  //
  typedef compiler::dispatcher<semantics::node> node_dispatcher;
  typedef compiler::dispatcher<semantics::edge> edge_dispatcher;

  //
  //
  struct node_base: node_dispatcher, edge_dispatcher
  {
    void
    edge_traverser (edge_dispatcher& d)
    {
      edge_dispatcher::traverser (d);
    }

    edge_dispatcher&
    edge_traverser ()
    {
      return *this;
    }

    using node_dispatcher::dispatch;
    using edge_dispatcher::dispatch;

    using edge_dispatcher::iterate_and_dispatch;
  };

  struct edge_base: edge_dispatcher, node_dispatcher
  {
    void
    node_traverser (node_dispatcher& d)
    {
      node_dispatcher::traverser (d);
    }

    node_dispatcher&
    node_traverser ()
    {
      return *this;
    }

    using edge_dispatcher::dispatch;
    using node_dispatcher::dispatch;

    using node_dispatcher::iterate_and_dispatch;
  };

  inline edge_base&
  operator>> (node_base& n, edge_base& e)
  {
    n.edge_traverser (e);
    return e;
  }

  inline node_base&
  operator>> (edge_base& e, node_base& n)
  {
    e.node_traverser (n);
    return n;
  }

  //
  //
  template <typename X>
  struct node: compiler::traverser_impl<X, semantics::node>,
               virtual node_base
  {
  };

  template <typename X>
  struct edge: compiler::traverser_impl<X, semantics::edge>,
               virtual edge_base
  {
  };

  //
  // Edges
  //

  struct names: edge<semantics::names>
  {
    names ()
    {
    }

    names (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct declares: edge<semantics::declares>
  {
    declares ()
    {
    }

    declares (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct defines: edge<semantics::defines>
  {
    defines ()
    {
    }

    defines (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct typedefs: edge<semantics::typedefs>
  {
    typedefs ()
    {
    }

    typedefs (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct belongs: edge<semantics::belongs>
  {
    belongs ()
    {
    }

    belongs (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  //
  // Nodes
  //

  struct nameable: node<semantics::nameable> {};

  //
  //
  template <typename T>
  struct scope_template: node<T>
  {
  public:
    virtual void
    traverse (T& s)
    {
      names (s);
    }

    virtual void
    names (T& s)
    {
      names (s, *this);
    }

    virtual void
    names (T& s, edge_dispatcher& d)
    {
      this->iterate_and_dispatch (s.names_begin (), s.names_end (), d);
    }
  };

  struct scope: scope_template<semantics::scope> {};

  //
  //
  struct type: node<semantics::type> {};

  //
  //
  struct instance: node<semantics::instance>
  {
    virtual void
    traverse (type&);

    virtual void
    belongs (type&);

    virtual void
    belongs (type&, edge_dispatcher&);
  };

  //
  //
  struct data_member: node<semantics::data_member>
  {
    virtual void
    traverse (type&);

    virtual void
    belongs (type&);

    virtual void
    belongs (type&, edge_dispatcher&);
  };

  //
  //
  struct unsupported_type: node<semantics::unsupported_type> {};
}

#endif // ODB_TRAVERSAL_ELEMENTS_HXX
