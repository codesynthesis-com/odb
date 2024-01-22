// file      : odb/traversal/relational/elements.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_ELEMENTS_HXX
#define ODB_TRAVERSAL_RELATIONAL_ELEMENTS_HXX

#include <libcutl/compiler/traversal.hxx>
#include <odb/semantics/relational/elements.hxx>

namespace traversal
{
  namespace relational
  {
    using namespace cutl;

    //
    //
    typedef compiler::dispatcher<semantics::relational::node> node_dispatcher;
    typedef compiler::dispatcher<semantics::relational::edge> edge_dispatcher;

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
    struct node: compiler::traverser_impl<X, semantics::relational::node>,
      virtual node_base
    {
      virtual void
      traverse (X&) {}
    };

    template <typename X>
    struct edge: compiler::traverser_impl<X, semantics::relational::edge>,
      virtual edge_base
    {
    };

    //
    // Edges
    //

    template <typename N>
    struct names: edge<semantics::relational::names<N> >
    {
      names ()
      {
      }

      names (node_dispatcher& n)
      {
        this->node_traverser (n);
      }

      virtual void
      traverse (semantics::relational::names<N>& e)
      {
        this->dispatch (e.nameable ());
      }
    };

    typedef names<semantics::relational::uname> unames;
    typedef names<semantics::relational::qname> qnames;

    //
    // Nodes
    //

    template <typename N>
    struct nameable: node<semantics::relational::nameable<N> > {};

    typedef nameable<semantics::relational::uname> unameable;
    typedef nameable<semantics::relational::qname> qnameable;

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

    template <typename N>
    struct scope: scope_template<semantics::relational::scope<N> > {};

    typedef scope<semantics::relational::uname> uscope;
    typedef scope<semantics::relational::qname> qscope;
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_ELEMENTS_HXX
