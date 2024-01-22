// file      : odb/semantics/unit.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_UNIT_HXX
#define ODB_SEMANTICS_UNIT_HXX

#include <map>

#include <odb/semantics/elements.hxx>
#include <odb/semantics/namespace.hxx>

namespace semantics
{
  class unit: public graph<node, edge>, public namespace_
  {
  public:
    unit (path const&);

  private:
    unit (unit const&);
    unit& operator= (unit const&);

    // Mapping from tree nodes to semantic graph nodes.
    //
  public:
    node*
    find (tree key) const
    {
      tree_node_map::const_iterator i (tree_node_map_.find (key));
      return i != tree_node_map_.end () ? i->second : 0;
    }

    void
    insert (tree key, node& value)
    {
      tree_node_map_[key] = &value;
    }

    using namespace_::find;

    // Mapping from tree nodes to name hints.
    //
  public:
    names*
    find_hint (tree key) const
    {
      name_hint_map::const_iterator i (name_hint_map_.find (key));
      return i != name_hint_map_.end () ? i->second : 0;
    }

    void
    insert_hint (tree key, names& name)
    {
      name_hint_map_[key] = &name;
    }

  public:
    template <class T>
    T&
    new_node (path const& file, size_t line, size_t column)
    {
      T& r (graph_.new_node<T> (file, line, column));
      r.unit (*this);
      return r;
    }

    template <class T, class A0>
    T&
    new_node (path const& file, size_t line, size_t column, A0 const& a0)
    {
      T& r (graph_.new_node<T> (file, line, column, a0));
      r.unit (*this);
      return r;
    }

    template <class T, class A0, class A1>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1)
    {
      T& r (graph_.new_node<T> (file, line, column, a0, a1));
      r.unit (*this);
      return r;
    }

    template <class T, class A0, class A1, class A2>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1, A2 const& a2)
    {
      T& r (graph_.new_node<T> (file, line, column, a0, a1, a2));
      r.unit (*this);
      return r;
    }

    template <class T, class A0, class A1, class A2, class A3>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1, A2 const& a2, A3 const& a3)
    {
      T& r (graph_.new_node<T> (file, line, column, a0, a1, a2, a3));
      r.unit (*this);
      return r;
    }

    template <class T, class A0, class A1, class A2, class A3, class A4>
    T&
    new_node (path const& file, size_t line, size_t column,
              A0 const& a0, A1 const& a1, A2 const& a2, A3 const& a3,
              A4 const& a4)
    {
      T& r (graph_.new_node<T> (file, line, column, a0, a1, a2, a3, a4));
      r.unit (*this);
      return r;
    }

    // For fundamental types.
    //
    template <class T>
    T&
    new_fund_node (tree tn)
    {
      T& r (graph_.new_node<T> (tn));
      r.unit (*this);
      return r;
    }

  protected:
    // Special names edge for the global namespace.
    //
    class global_names: public names
    {
    public:
      global_names ()
          : names ("")
      {
        scope_ = 0;
      }

      void
      set_left_node (unit&)
      {
      }

      void
      set_right_node (nameable& n)
      {
        named_ = &n;
      }

      void
      clear_left_node (unit&)
      {
      }

      void
      clear_right_node (nameable& n)
      {
        assert (named_ == &n);
        named_ = 0;
      }
    };

  public:
    void
    add_edge_left (global_names&)
    {
    }

    using namespace_::add_edge_right;

  private:
    graph<node, edge>& graph_;

    typedef std::map<tree, node*> tree_node_map;
    tree_node_map tree_node_map_;

    typedef std::map<tree, names*> name_hint_map;
    name_hint_map name_hint_map_;
  };
}

#endif // ODB_SEMANTICS_UNIT_HXX
