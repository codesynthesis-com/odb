// file      : odb/semantics/elements.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <libcutl/compiler/type-info.hxx>

#include <odb/cxx-lexer.hxx>

#include <odb/semantics/elements.hxx>
#include <odb/semantics/namespace.hxx>
#include <odb/semantics/unit.hxx>

using namespace std;

namespace semantics
{
  // access
  //
  static char const* access_str[] = {"public", "protected", "private"};

  char const* access::
  string () const
  {
    return access_str[value_];
  }

  //
  //
  node::
  node (path const& file, size_t line, size_t column, tree tn)
      : tree_node_ (tn), loc_ (file, line, column)
  {
  }

  node::
  node ()
      : loc_ (0)
  {
    // GCC plugin machinery #define's abort as a macro.
    //
    // std::abort ();
    abort ();
  }

  // nameable
  //

  bool nameable::
  in_scope (scope_type& s)
  {
    for (scope_type* p (&scope ());; p = &p->scope_ ())
    {
      //@@ Need to handle namespace extensions.
      //
      if (p == &s)
        return true;

      if (!p->named_p () || p->global_scope ())
        break;
    }

    return false;
  }

  bool nameable::
  anonymous_ () const
  {
    tree n (tree_node ());

    if (TYPE_P (n))
    {
      tree name (0);

      if (tree decl = TYPE_NAME (n))
        name = DECL_NAME (decl);

      return name != 0 && IDENTIFIER_ANON_P (name);
    }

    return true;
  }

  bool nameable::
  fq_anonymous_ (scope_entry const* prev) const
  {
    scope_entry scope (this, prev);

    // Nameable is fq-anonymous if all the paths to the global scope
    // have at least one anonymous link.
    //
    if (defined_ != 0 || !named_.empty ())
    {
      if (named ().global_scope ())
        return false;

      if (defined_ != 0)
      {
        nameable const& s (defined_->scope ());

        if (!scope.find (&s) && !s.fq_anonymous_ (&scope))
          return false;
      }

      for (names_list::const_iterator i (named_.begin ()), e (named_.end ());
           i != e; ++i)
      {
        nameable const& s ((*i)->scope ());

        if (!scope.find (&s) && !s.fq_anonymous_ (&scope))
          return false;
      }
    }

    // If we can get a literal name for this type node, then it is not
    // anonymous as long as its scope is not anonymous.
    //
    tree type (tree_node ());

    if (TYPE_P (type))
    {
      tree name (0);

      if (tree decl = TYPE_NAME (type))
      {
        name = DECL_NAME (decl);
        if (name != 0 && IDENTIFIER_ANON_P (name))
          return true;

        tree s (CP_DECL_CONTEXT (decl));

        gcc_tree_code_type tc (TREE_CODE (s));

        if (tc == TYPE_DECL)
          s = TREE_TYPE (s);
        else if (tc == NAMESPACE_DECL)
        {
          // "Unwind" any inline namespaces since they are not in
          // semantic grapth.
          //
          while (s != global_namespace)
          {
            tree prev (CP_DECL_CONTEXT (s));

#if BUILDING_GCC_MAJOR >= 8
            if (!is_nested_namespace (prev, s, true))
#else
            if (!is_associated_namespace (prev, s))
#endif
              break;

            s = prev;
          }
        }

        if (nameable* n = dynamic_cast<nameable*> (unit ().find (s)))
          return scope.find (n)  || n->fq_anonymous_ (&scope);
      }
      else
        return false; // Assume this is a derived type (e.g., pointer).
    }

    return true;
  }

  bool nameable::
  fq_anonymous (names* hint) const
  {
    if (hint != 0 || defined_ != 0)
    {
      names& n (hint ? *hint : *defined_);

      if (n.global_scope ())
        return false;

      return n.scope ().fq_anonymous ();
    }
    else
      return fq_anonymous ();
  }

  static string
  qualify_names (string const& n, bool qualify_first)
  {
    // @@ Creating a lexer for each call is a bad idea. Need
    //    to cache it somewhere.
    //
    cxx_string_lexer l;
    l.start (n);

    string r, t;
    bool punc (false);
    bool scoped (false);

    // Names returned by GCC's type_as_string() (on which this function
    // is called) include inline namespaces (e.g., std::__cxx11::string).
    // So, besides fully-qualifying names, this function also needs to get
    // rid of those. The idea is to resolve names as we lex them, skipping
    // inline namespaces and stopping once we reach something other than a
    // namespace.
    //
    tree ns (global_namespace);
    tree id;

    for (cpp_ttype tt = l.next (t, &id); tt != CPP_EOF; tt = l.next (t, &id))
    {
      if (punc && tt > CPP_LAST_PUNCTUATOR)
        r += ' ';

      punc = false;
      tree new_ns (global_namespace); // By default, revert to global.

      switch (static_cast<unsigned> (tt))
      {
      case CPP_LESS:
        {
          r += "< ";
          break;
        }
      case CPP_GREATER:
        {
          r += " >";
          break;
        }
      case CPP_COMMA:
        {
          r += ", ";
          break;
        }
      case CPP_NAME:
        {
          // Check if this is a namespace and, if so, whether it is
          // inline.
          //
          if (ns != 0)
          {
            new_ns = lookup_qualified_name (ns, id, false, false);

            if (new_ns == error_mark_node ||
                TREE_CODE (new_ns) != NAMESPACE_DECL)
              new_ns = 0; // Not a namespace, stop resolving.
            else
            {
              // Check if this is an inline namespace and skip it if so.
              //
#if BUILDING_GCC_MAJOR >= 8
              if (is_nested_namespace (ns, new_ns, true))
#else
              if (is_associated_namespace (ns, new_ns))
#endif
              {
                // Skip also the following scope operator. Strictly speaking
                // there could be none (i.e., this is a name of an inline
                // namespace) but we only use this function to print names
                // of anonymous types.
                //
                assert (l.next (t) == CPP_SCOPE);
                continue;
              }
            }
          }
          else
            new_ns = 0; // Keep it disabled until we hit a new name.

          // If the name was not preceeded with '::', qualify it.
          //
          if (!scoped)
          {
            if (!qualify_first)
              qualify_first = true;
            else
              r += "::";
          }

          r += t;
          punc = true;
          break;
        }
      case CPP_KEYWORD:
      case CPP_NUMBER:
        {
          r += t;
          punc = true;
          break;
        }
      case CPP_SCOPE:
        {
          new_ns = ns; // Don't change the namespace.
        }
        // Fall through.
      default:
        {
          r += t;
          break;
        }
      }

      scoped = (tt == CPP_SCOPE);
      ns = new_ns;
    }

    return r;
  }

  string nameable::
  name_ () const
  {
    tree n (tree_node ());

    if (!TYPE_P (n))
      return "<anonymous>";

    // @@ Doing this once and caching the result is probably a
    //    good idea.
    //
    return qualify_names (
      type_as_string (n, TFF_PLAIN_IDENTIFIER | TFF_UNQUALIFIED_NAME), false);
  }

  string nameable::
  fq_name () const
  {
    return fq_name_ (0);
  }

  string nameable::
  fq_name_ (scope_entry const* prev) const
  {
    // @@ Doing this once and caching the result is probably a
    //    good idea.
    //
    scope_entry scope (this, prev);

    if (named_p () && named ().global_scope ())
      return "";

    if (defined_ != 0)
    {
      nameable const& s (defined_->scope ());

      if (!scope.find (&s) && !s.fq_anonymous_ (&scope))
        return s.fq_name_ (&scope) + "::" + name ();
    }

    for (names_list::const_iterator i (named_.begin ()), e (named_.end ());
         i != e; ++i)
    {
      nameable const& s ((*i)->scope ());

      if (!scope.find (&s) && !s.fq_anonymous_ (&scope))
        return s.fq_name_ (&scope) + "::" + name ();
    }

    tree n (tree_node ());

    if (!TYPE_P (n))
      return "<anonymous>";

    return qualify_names (type_as_string (n, TFF_PLAIN_IDENTIFIER), true);
  }

  string nameable::
  fq_name (names* hint) const
  {
    if (hint != 0 || defined_ != 0)
    {
      names& n (hint ? *hint : *defined_);

      if (n.global_scope ())
        return "";

      return n.scope ().fq_name () + "::" + n.name ();
    }
    else
    {
      // Since there was no hint, prefer the literal name over the names
      // edges.
      //
      tree n (tree_node ());

      if (TYPE_P (n))
        return qualify_names (type_as_string (n, TFF_PLAIN_IDENTIFIER), true);

      // Last resort is to call the other version of fq_name which will
      // check the names edges.
      //
      return fq_name ();
    }
  }

  // scope
  //

  scope::names_iterator_pair scope::
  find (string const& name) const
  {
    names_map::const_iterator i (names_map_.find (name));

    if (i == names_map_.end ())
      return names_iterator_pair (names_.end (), names_.end ());
    else
      return names_iterator_pair (i->second.begin (), i->second.end ());
  }

  scope::names_iterator scope::
  find (names& e)
  {
    list_iterator_map::iterator i (iterator_map_.find (&e));
    return i != iterator_map_.end () ? i->second : names_.end ();
  }

  static bool
  is_base (type_id const& b, compiler::type_info const& d)
  {
    using compiler::type_info;

    for (type_info::base_iterator i (d.begin_base ());
         i != d.end_base (); ++i)
    {
      type_info const& ti (i->type_info ());

      if (b == ti.type_id () || is_base (b, ti))
        return true;
    }

    return false;
  }

  names* scope::
  lookup (string const& name,
          type_id const& ti,
          unsigned int flags,
          bool* hidden) const
  {
    names_iterator_pair p (find (name));
    names* r (0);

    for (names_const_iterator i (p.first); i != p.second; ++i)
    {
      type_id const& xti (typeid (i->named ()));

      // If types are equal, then we found a match. Also check if ti is
      // a base type of xti.
      //
      if (xti == ti || is_base (ti, compiler::lookup (xti)))
      {
        if (r != 0)
        {
          // If both are namespaces, then the one is just an extension
          // of the other.
          //
          if (!(r->named ().is_a<namespace_> () &&
                i->named ().is_a<namespace_> ()))
            throw ambiguous (*r, *i);
        }
        else
          r = &*i;
      }
    }

    if (r != 0)
      return r;

    // If we found a name but the types didn't match, then bail out
    // unless we want hidden names.
    //
    if (p.first != p.second)
    {
      if (hidden != 0)
        *hidden = true;

      if ((flags & include_hidden) == 0)
        return 0;
    }

    // Look in the outer scope unless requested not to or if this is
    // the global scope.
    //
    if ((flags & exclude_outer) == 0 && named_p () && !global_scope ())
      return scope ().lookup (name, ti, flags, hidden);

    return 0;
  }

  void scope::
  add_edge_left (names& e)
  {
    names_list::iterator i (names_.insert (names_.end (), &e));
    iterator_map_[&e] = i;
    names_map_[e.name ()].push_back (&e);
  }

  void scope::
  add_edge_left (names& e, names_iterator after)
  {
    names_list::iterator i;

    if (after.base () == names_.end ())
      i = names_.insert (names_.begin (), &e);
    else
    {
      names_list::iterator j (after.base ());
      i = names_.insert (++j, &e);
    }

    iterator_map_[&e] = i;
    names_map_[e.name ()].push_back (&e);
  }

  // type info
  //
  namespace
  {
    struct init
    {
      init ()
      {
        using compiler::type_info;

        // node
        //
        insert (type_info (typeid (node)));

        // edge
        //
        insert (type_info (typeid (edge)));

        // names
        //
        {
          type_info ti (typeid (names));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // declares
        //
        {
          type_info ti (typeid (declares));
          ti.add_base (typeid (names));
          insert (ti);
        }

        // defines
        //
        {
          type_info ti (typeid (defines));
          ti.add_base (typeid (declares));
          insert (ti);
        }

        // typedefs
        //
        {
          type_info ti (typeid (typedefs));
          ti.add_base (typeid (declares));
          insert (ti);
        }

        // nameable
        //
        {
          type_info ti (typeid (nameable));
          ti.add_base (typeid (node));
          insert (ti);
        }

        // scope
        //
        {
          type_info ti (typeid (scope));
          ti.add_base (typeid (nameable));
          insert (ti);
        }

        // type
        //
        {
          type_info ti (typeid (type));
          ti.add_base (typeid (nameable));
          insert (ti);
        }

        // belongs
        //
        {
          type_info ti (typeid (belongs));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // instance
        //
        {
          type_info ti (typeid (instance));
          ti.add_base (typeid (node));
          insert (ti);
        }

        // data_member
        //
        {
          type_info ti (typeid (data_member));
          ti.add_base (typeid (nameable));
          ti.add_base (typeid (instance));
          insert (ti);
        }

        // unsupported_type
        //
        {
          type_info ti (typeid (unsupported_type));
          ti.add_base (typeid (type));
          insert (ti);
        }
      }
    } init_;
  }
}
