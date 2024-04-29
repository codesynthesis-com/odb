// file      : odb/semantics/relational/elements.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_ELEMENTS_HXX
#define ODB_SEMANTICS_RELATIONAL_ELEMENTS_HXX

#include <map>
#include <list>
#include <vector>
#include <string>
#include <cassert>

#include <libcutl/container/graph.hxx>
#include <libcutl/container/pointer-iterator.hxx>
#include <libcutl/compiler/context.hxx>

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>

#include <odb/semantics/relational/name.hxx>

namespace semantics
{
  namespace relational
  {
    using namespace cutl;

    using std::string;

    using container::pointer_iterator;
    using compiler::context;

    typedef unsigned long long version;

    //
    //
    extern string const xmlns;

    //
    //
    class node;
    class edge;

    typedef container::graph<node, edge> graph;

    //
    //
    class edge: public context
    {
    public:
      template <typename X>
      bool
      is_a () const
      {
        return dynamic_cast<X const*> (this) != 0;
      }

    public:
      virtual
      ~edge () {}
    };

    //
    //
    class node: public context
    {
      // Return name of the node.
      //
    public:
      virtual string
      kind () const = 0;

    public:
      template <typename X>
      bool
      is_a () const
      {
        return dynamic_cast<X const*> (this) != 0;
      }

    public:
      virtual
      ~node () {}

      // XML serialization.
      //
      virtual void
      serialize (xml::serializer&) const = 0;

      // Sink functions that allow extensions in the form of one-way
      // edges.
      //
      void
      add_edge_right (edge&) {}

      void
      remove_edge_right (edge&) {}
    };

    //
    //
    class alters: public edge
    {
    public:
      node&
      base () const {return *base_;}

      node&
      modifier () const {return *modifier_;}

    public:
      alters () : base_ (0), modifier_ (0) {}

      void
      set_left_node (node& m)
      {
        assert (modifier_ == 0);
        modifier_ = &m;
      }

      void
      set_right_node (node& b)
      {
        assert (base_ == 0);
        base_ = &b;
      }

      void
      clear_left_node (node& m)
      {
        assert (modifier_ == &m);
        modifier_ = 0;
      }

      void
      clear_right_node (node& b)
      {
        assert (base_ == &b);
        base_ = 0;
      }

    protected:
      node* base_;
      node* modifier_;
    };

    //
    //
    template <typename N>
    class scope;

    template <typename N>
    class nameable;

    //
    //
    template <typename N>
    class names: public edge
    {
    public:
      typedef N name_type;
      typedef relational::scope<N> scope_type;
      typedef relational::nameable<N> nameable_type;

      name_type const&
      name () const
      {
        return name_;
      }

      scope_type&
      scope () const
      {
        return *scope_;
      }

      nameable_type&
      nameable () const
      {
        return *nameable_;
      }

    public:
      names (name_type const& name): name_ (name) {}

      void
      set_left_node (scope_type& n)
      {
        scope_ = &n;
      }

      void
      set_right_node (nameable_type& n)
      {
        nameable_ = &n;
      }

      void
      clear_left_node (scope_type& n)
      {
        assert (scope_ == &n);
        scope_ = 0;
      }

      void
      clear_right_node (nameable_type& n)
      {
        assert (nameable_ == &n);
        nameable_ = 0;
      }

    protected:
      name_type name_;
      scope_type* scope_;
      nameable_type* nameable_;
    };

    typedef names<uname> unames;
    typedef names<qname> qnames;

    //
    //
    template <typename N>
    class nameable: public virtual node
    {
    public:
      typedef N name_type;
      typedef relational::names<N> names_type;
      typedef relational::scope<N> scope_type;

      name_type const&
      name () const {return named_->name ();}

      scope_type&
      scope () const {return named ().scope ();}

      names_type&
      named () const {return *named_;}

      string const&
      id () const {return id_;}

    public:
      // Id identifies the C++ node (e.g., a class or a data member) that
      // this model node corresponds to. The ids are not necessarily unique
      // (e.g., there can be a foreign key and an index with the same id that
      // correspond to a container member). However, in any given scope, the
      // {id,typeid} must be unique. This becomes important when we try to
      // find correspondance between nodes during model diff'ing.
      //
      nameable (string const& id): id_ (id), named_ (0) {}

      virtual nameable&
      clone (scope_type&, graph&) const = 0;

      // Virtual because we call it via nameable interface (e.g., in copy).
      //
      virtual void
      add_edge_right (names_type& e)
      {
        assert (named_ == 0);
        named_ = &e;
      }

      virtual void
      remove_edge_right (names_type& e)
      {
        assert (named_ == &e);
        named_ = 0;
      }

      using node::add_edge_right;
      using node::remove_edge_right;

    protected:
      nameable (nameable const&, graph& g);
      nameable (xml::parser&, graph& g);

      void
      serialize_attributes (xml::serializer&) const;

    public:
      typedef void (*parser_func) (xml::parser&, scope_type&, graph&);
      typedef std::map<std::string, parser_func> parser_map;
      static parser_map parser_map_;

      template <typename T>
      static void
      parser_impl (xml::parser&, scope_type&, graph&);

    private:
      string id_;
      names_type* named_;
    };

    typedef nameable<uname> unameable;
    typedef nameable<qname> qnameable;


    //
    //
    struct duplicate_name
    {
      template <typename N>
      duplicate_name (relational::scope<N>&,
                      relational::nameable<N>& orig,
                      relational::nameable<N>& dup);

      node& scope;
      node& orig;
      node& dup;

      string name;
    };

    template <typename N>
    class scope: public virtual node
    {
    protected:
      typedef N name_type;
      typedef relational::names<N> names_type;
      typedef relational::nameable<N> nameable_type;

      typedef std::list<names_type*> names_list;
      typedef std::map<name_type, typename names_list::iterator> names_map;
      typedef
      std::map<names_type const*, typename names_list::iterator>
      names_iterator_map;

    public:
      typedef pointer_iterator<typename names_list::iterator> names_iterator;
      typedef
      pointer_iterator<typename names_list::const_iterator>
      names_const_iterator;

    public:
      // Iteration.
      //
      names_iterator
      names_begin ()
      {
        return names_.begin ();
      }

      names_iterator
      names_end ()
      {
        return names_.end ();
      }

      names_const_iterator
      names_begin () const
      {
        return names_.begin ();
      }

      names_const_iterator
      names_end () const
      {
        return names_.end ();
      }

      bool
      names_empty () const
      {
        return names_.empty ();
      }

      // Find (this scope only).
      //
      template <typename T>
      T*
      find (name_type const&);

      names_iterator
      find (name_type const&);

      names_const_iterator
      find (name_type const&) const;

      names_iterator
      find (names_type const&);

      names_const_iterator
      find (names_type const&) const;

      // Lookup in this and all altered scopes until we find what we are
      // looking for or hit a stop node of type S (e.g., drop_*).
      //
      template <typename T, typename S>
      T*
      lookup (name_type const&);

    public:
      scope*
      base () const
      {
        return alters_ != 0 ? &dynamic_cast<scope&> (alters_->base ()) : 0;
      }

    public:
      scope ()
          : first_key_ (names_.end ()),
            first_drop_column_ (names_.end ()),
            alters_ (0) {}

      // Virtual because we call it via scope interface (e.g., in copy).
      //
      virtual void
      add_edge_left (alters& a)
      {
        assert (alters_ == 0);
        alters_ = &a;
      }

      virtual void
      remove_edge_left (alters& a)
      {
        assert (alters_ == &a);
        alters_ = 0;
      }

      virtual void
      add_edge_left (names_type&);

      virtual void
      remove_edge_left (names_type&);

    protected:
      scope (scope const&, scope* base, graph&);
      scope (xml::parser&, scope* base, graph&);

      void
      serialize_content (xml::serializer&) const;

    protected:
      names_list names_;
      names_map names_map_;
      names_iterator_map iterator_map_;

      typename names_list::iterator first_key_;
      typename names_list::iterator first_drop_column_;

      alters* alters_;
    };

    template <>
    void scope<uname>::
    add_edge_left (names_type&);

    template <>
    void scope<uname>::
    remove_edge_left (names_type&);

    typedef scope<uname> uscope;
    typedef scope<qname> qscope;
  }
}

#include <odb/semantics/relational/elements.txx>

#endif // ODB_SEMANTICS_RELATIONAL_ELEMENTS_HXX
