// file      : odb/parser.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx> // Keep it first.

#include <set>
#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <sstream>
#include <iostream>

#include <odb/diagnostics.hxx>
#include <odb/parser.hxx>
#include <odb/semantics.hxx>

using namespace std;
using namespace semantics;

class parser::impl
{
public:
  typedef parser::failed failed;

  impl (options const&, loc_pragmas&, ns_loc_pragmas&, decl_pragmas&);

  unique_ptr<unit>
  parse (tree global_scope, path const& main_file);

private:
  typedef semantics::access access;

  // Extended GCC tree declaration that is either a GCC tree
  // declaration, a virtual declaration, or a pragma. If it is
  // a pragma, then the assoc flag indicated whether this pragma
  // has been associated with a declaration. Otherwise, the assoc
  // flag indicates whether pragmas have been associated with this
  // declaration (we use this to ignore certain declarations for
  // pragma association purposes, e.g., the anonymous type in
  // struct {...} m_).
  //
  struct tree_decl
  {
    tree decl;
    virt_declaration const* vdecl;
    pragma const* prag;
    mutable bool assoc; // Allow modification via std::set iterator.

    tree_decl (tree d): decl (d), vdecl (0), prag (0), assoc (false) {}
    tree_decl (virt_declaration const& d)
        : decl (0), vdecl (&d), prag (0), assoc (false) {}
    tree_decl (pragma const& p)
        : decl (0), vdecl (0), prag (&p), assoc (false) {}

    bool
    operator< (tree_decl const& y) const;
  };

  typedef multiset<tree_decl> decl_set;

private:
  void
  collect (tree ns);

  void
  emit ();

  // Emit a type declaration. This is either a named class-type definition/
  // declaration or a typedef. In the former case the function returns the
  // newly created type node. In the latter case it returns 0.
  //
  type*
  emit_type_decl (tree);

  // Emit a template declaration.
  //
  void
  emit_template_decl (tree);

  class_template&
  emit_class_template (tree, bool stub = false);

  union_template&
  emit_union_template (tree, bool stub = false);

  template <typename T>
  T&
  emit_class (tree, path const& f, size_t l, size_t c, bool stub = false);

  template <typename T>
  T&
  emit_union (tree, path const& f, size_t l, size_t c, bool stub = false);

  // Access is not used when creating a stub.
  //
  enum_&
  emit_enum (tree,
             access,
             path const& f,
             size_t l,
             size_t c,
             bool stub = false);

  // Create new or find existing semantic graph type.
  //
  type&
  emit_type (tree, access, path const& f, size_t l, size_t c);

  type&
  create_type (tree, access, path const& f, size_t l, size_t c);

  string
  emit_type_name (tree, bool direct = true);


  //
  // Pragma handling.
  //
  void
  add_pragma (node&, pragma const&);

  // Process positioned and named pragmas.
  //
  void
  process_pragmas (declaration const&,
                   node&,
                   string const& name,
                   decl_set::const_iterator begin,
                   decl_set::const_iterator cur,
                   decl_set::const_iterator end);

  // Process named pragmas only.
  //
  void
  process_named_pragmas (declaration const&, node&);

  void
  diagnose_unassoc_pragmas (decl_set const&);

  // Return declaration's fully-qualified scope name (e.g., ::foo::bar).
  //
  string
  fq_scope (tree);

  // Return declaration's access.
  //
  access
  decl_access (tree decl)
  {
    // Note that TREE_PUBLIC() returns something other than what we need.
    //
    if (TREE_PRIVATE (decl))
      return access::private_;

    if (TREE_PROTECTED (decl))
      return access::protected_;

    return access::public_;
  }

  //
  //
  template <typename T>
  void
  define_fund (tree);

private:
  options const& ops_;
  loc_pragmas& loc_pragmas_;
  ns_loc_pragmas& ns_loc_pragmas_;
  decl_pragmas& decl_pragmas_;

  bool trace;
  ostream& ts;

  unit* unit_;
  scope* scope_;
  vector<scope*> class_scopes_; // Current hierarchy of class-like scopes.
  size_t error_;

  decl_set decls_;

  typedef map<location_t, tree> decl_map;
  decl_map all_decls_;
};

bool parser::impl::tree_decl::
operator< (tree_decl const& y) const
{
  location_t xl, yl;
  int xb (0), yb (0);

  if (decl != 0)
    xl = real_source_location (decl);
  else if (vdecl != 0)
  {
    xl = vdecl->ord;
    xb = vdecl->ord_bias;
  }
  else
    xl = prag->loc;

  if (y.decl != 0)
    yl = real_source_location (y.decl);
  else if (y.vdecl != 0)
  {
    yl = y.vdecl->ord;
    yb = y.vdecl->ord_bias;
  }
  else
    yl = y.prag->loc;

  // If both are virtual and at the same location, use the definition
  // location to order them.
  //
  if (vdecl != 0 && y.vdecl != 0 && xl == yl && xb == yb)
  {
    xl = vdecl->loc;
    yl = y.vdecl->loc;
  }

  return xl < yl || (xl == yl && xb < yb);
}

//
// Function templates.
//

template <typename T>
void parser::impl::
define_fund (tree t)
{
  t = TYPE_MAIN_VARIANT (t);
  char const* name (IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (t))));

  T& node (unit_->new_fund_node<T> (t));
  unit_->new_edge<defines> (*scope_, node, name);
  unit_->insert (t, node);

  process_named_pragmas (t, node);
}

template <typename T>
T& parser::impl::
emit_class (tree c, path const& file, size_t line, size_t clmn, bool stub)
{
  c = TYPE_MAIN_VARIANT (c);

  // See if there is a stub already for this type.
  //
  T* c_node (0);

  if (node* n = unit_->find (c))
  {
    c_node = &dynamic_cast<T&> (*n);
  }
  else
  {
    c_node = &unit_->new_node<T> (file, line, clmn, c);
    unit_->insert (c, *c_node);
  }

  if (stub || !COMPLETE_TYPE_P (c))
    return *c_node;

  // Note: "include" the base classes into the class scope (see comment for
  // self-typedefs in emit_type_decl()).
  //
  class_scopes_.push_back (c_node);

  // Traverse base information.
  //
  tree bis (TYPE_BINFO (c));
  size_t n (bis ? BINFO_N_BASE_BINFOS (bis) : 0);

  for (size_t i (0); i < n; i++)
  {
    tree bi (BINFO_BASE_BINFO (bis, i));
    access a (access::public_);

    if (BINFO_BASE_ACCESSES (bis))
    {
      tree ac (BINFO_BASE_ACCESS (bis, i));

      if (ac == NULL_TREE || ac == access_public_node)
      {
        a = access::public_;
      }
      else if (ac == access_protected_node)
      {
        a = access::protected_;
      }
      else
      {
        assert (ac == access_private_node);
        a = access::private_;
      }
    }

    bool virt (BINFO_VIRTUAL_P (bi));
    tree base (TYPE_MAIN_VARIANT (BINFO_TYPE (bi)));

    // Find the corresponding graph node. If we cannot find one then
    // the base is a template instantiation since an ordinary class
    // has to be defined (complete) in order to be a base.
    //
    class_* b_node (0);
    string name;

    if (node* n = unit_->find (base))
    {
      b_node = &dynamic_cast<class_&> (*n);

      if (trace)
        name = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (base)));
    }
    else
    {
      // Use public access for a template instantiation in the inheritance
      // declaration.
      //
      b_node = &dynamic_cast<class_&> (
        emit_type (base, access::public_, file, line, clmn));

      if (trace)
        name = emit_type_name (base);
    }

    unit_->new_edge<inherits> (*c_node, *b_node, a, virt);

    if (trace)
      ts << "\t" << a.string () << (virt ? " virtual" : "") << " base "
         << name << " (" << static_cast<type*> (b_node) << ")" << endl;
  }

  // Collect member declarations so that we can traverse them in
  // the source code order.
  //
  decl_set decls;

  for (tree d (TYPE_FIELDS (c)); d != NULL_TREE; d = TREE_CHAIN (d))
  {
    switch (TREE_CODE (d))
    {
    case TYPE_DECL:
      {
        if (!DECL_SELF_REFERENCE_P (d))
          decls.insert (d);
        break;
      }
    case TEMPLATE_DECL:
      {
        if (DECL_CLASS_TEMPLATE_P (d))
          decls.insert (d);
        break;
      }
    case FIELD_DECL:
      {
        if (!DECL_ARTIFICIAL (d))
          decls.insert (d);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  // Add virtual declarations if any.
  //
  {
    virt_declarations::const_iterator i (virt_declarations_.find (c));

    if (i != virt_declarations_.end ())
      decls.insert (i->second.begin (), i->second.end ());
  }

  // Add location pragmas if any.
  //
  {
    loc_pragmas::const_iterator i (loc_pragmas_.find (c));

    if (i != loc_pragmas_.end ())
      decls.insert (i->second.begin (), i->second.end ());
  }

  scope* prev_scope (scope_);
  scope_ = c_node;

  for (decl_set::const_iterator b (decls.begin ()), i (b), e (decls.end ());
       i != e; ++i)
  {
    // Skip pragmas.
    //
    if (i->prag != 0)
      continue;

    // Handle virtual declarations.
    //
    if (i->vdecl != 0)
    {
      virt_declaration const& vd (*i->vdecl);

      switch (vd.tree_code)
      {
      case FIELD_DECL:
        {
          // First check that it doesn't conflict with any of the real
          // data members defined in this class.
          //
          tree d (
            lookup_qualified_name (
              c, get_identifier (vd.name.c_str ()), false, false));

          if (d != error_mark_node && TREE_CODE (d) == FIELD_DECL)
          {
            error (vd.loc) << "virtual data member declaration '" << vd.name
                           << "' conflicts with a previous declaration"
                           << endl;

            location_t l (real_source_location (d));
            info (l) << "'" << vd.name << "' was previously declared here"
                     << endl;

            throw failed ();
          }

          path file (LOCATION_FILE (vd.loc));
          size_t line (LOCATION_LINE (vd.loc));
          size_t clmn (LOCATION_COLUMN (vd.loc));

          access a (access::public_);

          type& type_node (emit_type (vd.type, a, file, line, clmn));
          data_member& member_node (
            unit_->new_node<data_member> (file, line, clmn, tree (0)));

          unit_->new_edge<names> (*c_node, member_node, vd.name, a);
          belongs& edge (unit_->new_edge<belongs> (member_node, type_node));

          // See if there is a name hint for this type.
          //
          if (names* hint = unit_->find_hint (vd.type))
            edge.hint (*hint);

          // Process pragmas that may be associated with this field.
          //
          process_pragmas (vd, member_node, vd.name, b, i, e);
          break;
        }
      default:
        {
          assert (false);
          break;
        }
      }
      continue;
    }

    tree d (i->decl);

    switch (TREE_CODE (d))
    {
    case TYPE_DECL:
      {
        type* n (emit_type_decl (d));

        // If this is a named class-type definition, then handle
        // the pragmas.
        //
        if (n != 0)
          process_pragmas (n->tree_node (), *n, n->name (), b, i, e);

        break;
      }
    case TEMPLATE_DECL:
      {
        emit_template_decl (d);
        break;
      }
    case FIELD_DECL:
      {
        // If this is a bit-field then TREE_TYPE may be a modified type
        // with lesser precision. In this case, DECL_BIT_FIELD_TYPE
        // will be the type that was original specified. Use that type
        // for now. Furthermore, bitfields can be anonymous, which we
        // ignore.
        //
        //
        bool bf (DECL_C_BIT_FIELD (d));

        if (bf && DECL_NAME (d) == 0)
          break;

        // Another case where we can have NULL name is anonymous struct
        // or union extension, for example:
        //
        // struct s
        // {
        //   union
        //   {
        //     int a;
        //     int b;
        //   };
        // };
        //
        // GCC appears to create a fake member for such a struct/union
        // without any name. Ignore such members for now.
        //
        if (DECL_NAME (d) == 0)
          break;

        tree t (bf ? DECL_BIT_FIELD_TYPE (d) : TREE_TYPE (d));

        char const* name (IDENTIFIER_POINTER (DECL_NAME (d)));

        path file (DECL_SOURCE_FILE (d));
        size_t line (DECL_SOURCE_LINE (d));
        size_t clmn (DECL_SOURCE_COLUMN (d));

        access a (decl_access (d));

        type& type_node (emit_type (t, a, file, line, clmn));
        data_member& member_node (
          unit_->new_node<data_member> (file, line, clmn, d));
        unit_->insert (d, member_node);

        unit_->new_edge<names> (*c_node, member_node, name, a);
        belongs& edge (unit_->new_edge<belongs> (member_node, type_node));

        // See if there is a name hint for this type.
        //
        if (names* hint = unit_->find_hint (t))
          edge.hint (*hint);

        if (trace)
        {
          string type_name (emit_type_name (t));

          ts << "\t" << a.string () << " data member " << type_name
             << " (" << &type_node << ") " << name << " at "
             << file << ":" << line << endl;
        }

        // Process pragmas that may be associated with this field.
        //
        process_pragmas (d, member_node, name, b, i, e);

        break;
      }
    default:
      {
        assert (false);
        break;
      }
    }
  }

  // Diagnose any position pragmas that haven't been associated.
  //
  diagnose_unassoc_pragmas (decls);

  scope_ = prev_scope;
  class_scopes_.pop_back ();

  return *c_node;
}

template <typename T>
T& parser::impl::
emit_union (tree u, path const& file, size_t line, size_t clmn, bool stub)
{
  u = TYPE_MAIN_VARIANT (u);

  // See if there is a stub already for this type.
  //
  T* u_node (0);

  if (node* n = unit_->find (u))
  {
    u_node = &dynamic_cast<T&> (*n);
  }
  else
  {
    u_node = &unit_->new_node<T> (file, line, clmn, u);
    unit_->insert (u, *u_node);
  }

  if (stub || !COMPLETE_TYPE_P (u))
    return *u_node;

  class_scopes_.push_back (u_node);

  // Collect member declarations so that we can traverse them in
  // the source code order.
  //
  decl_set decls;

  for (tree d (TYPE_FIELDS (u)); d != NULL_TREE ; d = TREE_CHAIN (d))
  {
    switch (TREE_CODE (d))
    {
    case TYPE_DECL:
      {
        if (!DECL_SELF_REFERENCE_P (d))
          decls.insert (d);
        break;
      }
    case TEMPLATE_DECL:
      {
        if (DECL_CLASS_TEMPLATE_P (d))
          decls.insert (d);
        break;
      }
    case FIELD_DECL:
      {
        if (!DECL_ARTIFICIAL (d))
          decls.insert (d);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  // Add location pragmas if any.
  //
  {
    loc_pragmas::const_iterator i (loc_pragmas_.find (u));

    if (i != loc_pragmas_.end ())
      decls.insert (i->second.begin (), i->second.end ());
  }

  scope* prev_scope (scope_);
  scope_ = u_node;

  for (decl_set::const_iterator b (decls.begin ()), i (b), e (decls.end ());
       i != e; ++i)
  {
    // Skip pragmas.
    //
    if (i->prag)
      continue;

    tree d (i->decl);

    switch (TREE_CODE (d))
    {
    case TYPE_DECL:
      {
        type* n (emit_type_decl (d));

        // If this is a named class-type definition, then handle
        // the pragmas.
        //
        if (n != 0)
          process_pragmas (n->tree_node (), *n, n->name (), b, i, e);

        break;
      }
    case TEMPLATE_DECL:
      {
        emit_template_decl (d);
        break;
      }
    case FIELD_DECL:
      {
        // We can have NULL name in case of anonymous struct or union
        // extension, for example:
        //
        // union s
        // {
        //   struct
        //   {
        //     int a;
        //     int b;
        //   };
        //   int c;
        // };
        //
        // GCC appears to create a fake member for such a struct/union
        // without any name. Ignore such members for now.
        //
        if (DECL_NAME (d) == 0)
          break;

        tree t (TREE_TYPE (d));
        char const* name (IDENTIFIER_POINTER (DECL_NAME (d)));

        path file (DECL_SOURCE_FILE (d));
        size_t line (DECL_SOURCE_LINE (d));
        size_t clmn (DECL_SOURCE_COLUMN (d));

        access a (decl_access (d));

        type& type_node (emit_type (t, a, file, line, clmn));
        data_member& member_node (
          unit_->new_node<data_member> (file, line, clmn, d));
        unit_->insert (d, member_node);

        unit_->new_edge<names> (*u_node, member_node, name, a);
        belongs& edge (unit_->new_edge<belongs> (member_node, type_node));

        // See if there is a name hint for this type.
        //
        if (names* hint = unit_->find_hint (t))
          edge.hint (*hint);

        if (trace)
        {
          string type_name (emit_type_name (t));

          ts << "\t" << a.string () << " union member " << type_name
             << " (" << &type_node << ") " << name << " at "
             << file << ":" << line << endl;
        }

        // Process pragmas that may be associated with this field.
        //
        process_pragmas (d, member_node, name, b, i, e);

        break;
      }
    default:
      {
        break;
      }
    }
  }

  // Diagnose any position pragmas that haven't been associated.
  //
  diagnose_unassoc_pragmas (decls);

  scope_ = prev_scope;
  class_scopes_.pop_back ();
  return *u_node;
}

//
// Functions.
//

parser::impl::
impl (options const& ops,
      loc_pragmas & lp,
      ns_loc_pragmas& nslp,
      decl_pragmas& dp)
    : ops_ (ops),
      loc_pragmas_ (lp),
      ns_loc_pragmas_ (nslp),
      decl_pragmas_ (dp),
      trace (ops.trace ()),
      ts (cerr)
{
}

unique_ptr<unit> parser::impl::
parse (tree global_scope, path const& main_file)
{
  unique_ptr<unit> u (new unit (main_file));
  u->insert (global_namespace, *u);
  process_named_pragmas (global_namespace, *u);

  unit_ = u.get ();
  scope_ = unit_;
  error_ = 0;

  // Define fundamental types.
  //
  define_fund<fund_void> (void_type_node);
  define_fund<fund_bool> (boolean_type_node);
  define_fund<fund_char> (char_type_node);
  define_fund<fund_wchar> (wchar_type_node);

  if (ops_.std () >= cxx_version::cxx11)
  {
    define_fund<fund_char16> (char16_type_node);
    define_fund<fund_char32> (char32_type_node);
  }

  define_fund<fund_signed_char> (signed_char_type_node);
  define_fund<fund_unsigned_char> (unsigned_char_type_node);
  define_fund<fund_short> (short_integer_type_node);
  define_fund<fund_unsigned_short> (short_unsigned_type_node);
  define_fund<fund_int> (integer_type_node);
  define_fund<fund_unsigned_int> (unsigned_type_node);
  define_fund<fund_long> (long_integer_type_node);
  define_fund<fund_unsigned_long> (long_unsigned_type_node);
  define_fund<fund_long_long> (long_long_integer_type_node);
  define_fund<fund_unsigned_long_long> (long_long_unsigned_type_node);
  define_fund<fund_float> (float_type_node);
  define_fund<fund_double> (double_type_node);
  define_fund<fund_long_double> (long_double_type_node);

  // First collect all the namespace-level declarations we are
  // interested in in the line-decl map so that they appear in
  // the source code order.
  //
  collect (global_scope);

  // Add namespace-level position pragmas if any.
  //
  {
    loc_pragmas::const_iterator i (loc_pragmas_.find (global_namespace));

    if (i != loc_pragmas_.end ())
      decls_.insert (i->second.begin (), i->second.end ());
  }

  // Convert position namespace pragmas to name pragmas.
  //
  for (ns_loc_pragmas::const_iterator i (ns_loc_pragmas_.begin ());
       i != ns_loc_pragmas_.end (); ++i)
  {
    pragma const& p (i->pragma);

    decl_map::const_iterator j (all_decls_.lower_bound (p.loc));

    if (j == all_decls_.end ())
    {
      error (p.loc)
        << "db pragma '" << p.pragma_name << "' is not associated with a "
        << "namespace declaration" << endl;
      error_++;
      continue;
    }

    // Find the "namespace difference" between this declaration and
    // the pragma's namespace. The outermost namespace in the result
    // is what we are looking for.
    //
    tree ns (0);

    for (tree prev (j->second), scope (CP_DECL_CONTEXT (prev));;
         scope = CP_DECL_CONTEXT (scope))
    {
      if (scope == i->ns)
      {
        ns = prev;
        break;
      }

      if (scope == global_namespace)
        break;

      prev = scope;
    }

    if (ns == 0 || TREE_CODE (ns) != NAMESPACE_DECL)
    {
      error (p.loc)
        << "db pragma '" << p.pragma_name << "' is not associated with a "
        << "namespace declaration" << endl;
      error_++;
      continue;
    }

    pragma_set& s (decl_pragmas_[ns]);
    pragma_set::iterator it (s.find (p.context_name));

    // Make sure we override only if this pragma came after the one
    // already in the set.
    //
    if (it == s.end () || it->second.loc <= p.loc)
      s.insert (p);
  }

  // Construct the semantic graph.
  //
  if (error_ == 0)
    emit ();

  if (error_ > 0)
    throw failed ();

  return u;
}

void parser::impl::
collect (tree ns)
{
  cp_binding_level* level = NAMESPACE_LEVEL (ns);
  tree decl = level->names;

  // Collect declarations.
  //
  for (; decl != NULL_TREE; decl = TREE_CHAIN (decl))
  {
    all_decls_[real_source_location (decl)] = decl;

    if (DECL_IS_BUILTIN (decl))
      continue;

    switch (TREE_CODE (decl))
    {
    case TYPE_DECL:
      {
        // Skip special type declarations.
        //
        if (DECL_NAME (decl) == NULL_TREE)
          continue;

        tree type (TREE_TYPE (decl));
        if (LAMBDA_TYPE_P (type))
          continue;

        decls_.insert (decl);
        break;
      }
    case TEMPLATE_DECL:
      {
        if (DECL_CLASS_TEMPLATE_P (decl))
          decls_.insert (decl);

        break;
      }
    default:
      {
        break;
      }
    }
  }

  // Traverse namespaces.
  //
  for (
#if BUILDING_GCC_MAJOR >= 8
    decl = level->names;
#else
    decl = level->namespaces;
#endif
    decl != NULL_TREE;
    decl = TREE_CHAIN (decl))
  {
#if BUILDING_GCC_MAJOR >= 8
    // Now namespaces are interleaved with other declarations. In fact, we
    // could probably collect everything in a single pass.
    //
    if (TREE_CODE (decl) != NAMESPACE_DECL)
      continue;
#endif

    // Ignore namespace aliases.
    //
    if (DECL_NAMESPACE_ALIAS (decl))
      continue;

    if (!DECL_IS_BUILTIN (decl) || DECL_NAMESPACE_STD_P (decl))
    {
      tree dn (DECL_NAME (decl));

      if (trace)
      {
        char const* name (dn ? IDENTIFIER_POINTER (dn) : "<anonymous>");

        ts << "namespace " << name << " at "
           << DECL_SOURCE_FILE (decl) << ":"
           << DECL_SOURCE_LINE (decl) << endl;
      }

      // Skip anonymous namespaces (there could be nothing of interest to us
      // inside but they wreck havoc with our attempts to sort declarations
      // into namespaces).
      //
      if (dn != 0)
        collect (decl);
    }
  }
}

void parser::impl::
emit ()
{
  for (decl_set::const_iterator b (decls_.begin ()), i (b),
         e (decls_.end ()); i != e; ++i)
  {
    // Skip pragmas.
    //
    if (i->prag)
      continue;

    tree decl (i->decl);

    // Get this declaration's namespace and unwind our scope until
    // we find a common prefix of namespaces.
    //
    string pfx;
    string ns (fq_scope (decl));

    for (pfx = scope_->fq_name (); !pfx.empty (); pfx = scope_->fq_name ())
    {
      size_t n (pfx.size ());

      // Make sure we handle cases like ns="::foobar", pfx="::foo".
      //
      if (ns.compare (0, n, pfx) == 0 && (ns.size () == n || ns[n - 1] == ':'))
        break;

      if (trace)
        ts << "closing namespace " << scope_->name () << endl;

      scope_ = &scope_->scope_ ();
    }

    // Build the rest of the namespace hierarchy for this declaration.
    //
    if (ns != pfx)
    {
      path f (DECL_SOURCE_FILE (decl));
      size_t l (DECL_SOURCE_LINE (decl));
      size_t c (DECL_SOURCE_COLUMN (decl));

      for (size_t b (pfx.size () + 2), e (ns.find ("::", b));
           b != string::npos;)
      {
        string n (ns, b, e == string::npos ? e : e - b);

        if (trace)
          ts << "opening namespace " << n << " for "
             << DECL_SOURCE_FILE (decl) << ":"
             << DECL_SOURCE_LINE (decl) << endl;

        // Use the declarations's file, line, and column as an
        // approximation for this namespace origin. Also resolve
        // the tree node for this namespace.
        //
#if BUILDING_GCC_MAJOR >= 8
        tree tree_node (
          get_namespace_binding (
            scope_->tree_node (), get_identifier (n.c_str ())));
#else
        tree tree_node (
          namespace_binding (
            get_identifier (n.c_str ()), scope_->tree_node ()));
#endif

        namespace_& node (unit_->new_node<namespace_> (f, l, c, tree_node));
        unit_->new_edge<defines> (*scope_, node, n);

        if (namespace_* orig =
            dynamic_cast<namespace_*> (unit_->find (tree_node)))
        {
          // This is an extension.
          //
          node.original (*orig);
        }
        else
        {
          // This is the original. Add it to the map and process any
          // pragmas it might have (at this stage namespaces can only
          // have name pragmas).
          //
          unit_->insert (tree_node, node);
          process_named_pragmas (tree_node, node);
        }

        scope_ = &node;

        if (e == string::npos)
          b = e;
        else
        {
          b = e + 2;
          e = ns.find ("::", b);
        }
      }
    }

    switch (TREE_CODE (decl))
    {
    case TYPE_DECL:
      {
        type* n (emit_type_decl (decl));

        // If this is a named class-type definition, then handle
        // the pragmas.
        //
        if (n != 0)
          process_pragmas (n->tree_node (), *n, n->name (), b, i, e);

        break;
      }
    case TEMPLATE_DECL:
      {
        emit_template_decl (decl);
        break;
      }
    default:
      {
        break;
      }
    }

    assert (class_scopes_.empty ());
  }

  // Diagnose any position pragmas that haven't been associated.
  //
  diagnose_unassoc_pragmas (decls_);
}

type* parser::impl::
emit_type_decl (tree decl)
{
  tree t (TREE_TYPE (decl));
  gcc_tree_code_type tc (TREE_CODE (t));

  tree decl_name (DECL_NAME (decl));
  char const* name (IDENTIFIER_POINTER (decl_name));

  if (DECL_ARTIFICIAL (decl) &&
      (tc == RECORD_TYPE || tc == UNION_TYPE || tc == ENUMERAL_TYPE))
  {
    // If we have an anonymous class typedef, use the user-
    // supplied name instead of the synthesized one. ARM
    // says that in typedef struct {} S; S becomes struct's
    // name.
    //
    if (IDENTIFIER_ANON_P (decl_name))
    {
      tree d (TYPE_NAME (t));

      if (d != NULL_TREE &&
          !DECL_ARTIFICIAL (d) &&
          DECL_NAME (d) != NULL_TREE &&
          !IDENTIFIER_ANON_P (DECL_NAME (d)))
      {
        decl = d;
        decl_name = DECL_NAME (decl);
        name = IDENTIFIER_POINTER (decl_name);
      }
      else
      {
        // This type has only the synthesized name which means that
        // it is either typedef'ed as a derived type or it is used
        // to declare a varibale or similar. The first case will be
        // covered by the typedef handling code below. The second
        // case will be covere by emit_type().
        //
        return 0;
      }
    }

    path file (DECL_SOURCE_FILE (decl));
    size_t line (DECL_SOURCE_LINE (decl));
    size_t clmn (DECL_SOURCE_COLUMN (decl));

    type* node (0);

    // Pointers to member functions are represented as record
    // types. Detect and handle this case.
    //
    if (TYPE_PTRMEMFUNC_P (t))
    {
      t = TYPE_MAIN_VARIANT (t);
      node = &unit_->new_node<unsupported_type> (
        file, line, clmn, t, "pointer_to_member_function_type");
      unit_->insert (t, *node);
    }
    else
    {

      if (trace)
        ts << "start " <<  gcc_tree_code_name(tc) << " " << name
           << " at " << file << ":" << line << endl;

      switch (tc)
      {
      case RECORD_TYPE:
        {
          node = &emit_class<class_> (t, file, line, clmn);
          break;
        }
      case UNION_TYPE:
        {
          node = &emit_union<union_> (t, file, line, clmn);
          break;
        }
      case ENUMERAL_TYPE:
        {
          node = &emit_enum (t, decl_access (decl), file, line, clmn);
          break;
        }
      default:
        break;
      }

      if (trace)
        ts << "end " <<  gcc_tree_code_name(tc) << " " << name
           << " (" << node << ") at "
           << DECL_SOURCE_FILE (decl) << ":"
           << DECL_SOURCE_LINE (decl) << endl;
    }

    if (COMPLETE_TYPE_P (t))
      unit_->new_edge<defines> (*scope_, *node, name);
    else
      unit_->new_edge<declares> (*scope_, *node, name);

    return node;
  }
  else
  {
    // Normal typedef. We need to detect and ignore the anonymous
    // class typedef case described above since we already used
    // this name to define the class.
    //
    if ((tc == RECORD_TYPE || tc == UNION_TYPE || tc == ENUMERAL_TYPE) &&
        TYPE_NAME (TYPE_MAIN_VARIANT (t)) == decl)
      return 0;

    path f (DECL_SOURCE_FILE (decl));
    size_t l (DECL_SOURCE_LINE (decl));
    size_t c (DECL_SOURCE_COLUMN (decl));

    type& node (emit_type (t, decl_access (decl), f, l, c));

    // Omit inner self-typedefs (e.g., a class typedefs itself in its own
    // scope). Such aliases don't buy us anything (in particular, they cannot
    // be used to form an fq-name) but they do cause scoping cycles if this
    // name happens to be used to find outer scope (see scope::scope_()).
    // Note that this means we can now have class template instantiations that
    // are not named and therefore don't belong to any scope.
    //
    // Note that emit_type() might still enter this decl as a hint. It's fuzzy
    // whether this is harmless or not.
    //
    // Note also that using the normal scope hierarchy does not work in more
    // complex cases where templates cross-self-typedef. So instead we now use
    // a special-purpose mechanism (class_scopes_). Note for this to work
    // correctly (hopefully), the class should be "in scope" for its bases.
    // Here is a representative examples (inspired by code in Eigen):
    //
    // template <typename M>
    // struct PlainObjectBase
    // {
    //   typedef M Self;
    // };
    //
    // template <typename T, int X, int Y>
    // struct Matrix: PlainObjectBase<Matrix<T, X, Y>>
    // {
    //   typedef PlainObjectBase<Matrix> Base;
    //   typedef Matrix Self;
    // };
    //
    // typedef Matrix<double, 3, 1> Vector3d;
    //
    // Here we want both Self's (but not Base) to be skipped.
    //
    if (scope* s = dynamic_cast<scope*> (&node))
    {
      for (auto i (class_scopes_.rbegin ()); i != class_scopes_.rend (); ++i)
      {
        if (s == *i)
        {
          if (trace)
          {
            string s (emit_type_name (t, false));

            ts << "omitting inner self-typedef " << s << " (" << &node
               << ") -> " << name << " at " << f << ":" << l << endl;
          }
          return 0;
        }
      }
    }

    typedefs& edge (unit_->new_edge<typedefs> (*scope_, node, name));

    // Find our hint.
    //
    if (tree ot = DECL_ORIGINAL_TYPE (decl))
    {
      if (names* hint = unit_->find_hint (ot))
        edge.hint (*hint);
    }

    // Add this edge to the hint map. It may already be there if we
    // are handling something like this:
    //
    // typedef foo bar;
    // typedef bar foo;
    //
    // GCC also appears to re-purpose a node for another name (not
    // sure if its a bug or a feature), so use the latest seen name.
    //
    unit_->insert_hint (t, edge);

    if (trace)
    {
      string s (emit_type_name (t, false));

      ts << "typedef " << s << " (" << &node << ") -> " << name
         << " at " << f << ":" << l << endl;
    }

    return 0;
  }
}

void parser::impl::
emit_template_decl (tree decl)
{
  // Currently we only handle class/union templates.
  //
  tree t (TREE_TYPE (DECL_TEMPLATE_RESULT (decl)));
  gcc_tree_code_type tc (TREE_CODE (t));

  if (trace)
  {
    ts << gcc_tree_code_name(tc) << " template (" << decl << ") "
       << IDENTIFIER_POINTER (DECL_NAME (decl)) << " (" << t << ") at "
       << DECL_SOURCE_FILE (decl) << ":"
       << DECL_SOURCE_LINE (decl) << endl;

    ts << "specializations:" << endl;

    for (tree s (DECL_TEMPLATE_SPECIALIZATIONS (decl));
         s != NULL_TREE; s = TREE_CHAIN (s))
    {
      tree t (TREE_TYPE (s));
      tree d (TYPE_NAME (t));

      ts << "\tspecialization " << t << " at "
         << DECL_SOURCE_FILE (d) << ":"
         << DECL_SOURCE_LINE (d) << endl;
    }

    ts << "instantiations:" << endl;

    for (tree i (DECL_TEMPLATE_INSTANTIATIONS (decl));
         i != NULL_TREE; i = TREE_CHAIN (i))
    {
      tree t (TREE_VALUE (i));
      tree d (TYPE_NAME (t));

      ts << "\tinstantiation " << t << " at "
         << DECL_SOURCE_FILE (d) << ":"
         << DECL_SOURCE_LINE (d) << endl;
    }
  }

  char const* name (IDENTIFIER_POINTER (DECL_NAME (decl)));

  if (trace)
    ts << "start " <<  gcc_tree_code_name(tc) << " template " << name << " at "
       << DECL_SOURCE_FILE (decl) << ":"
       << DECL_SOURCE_LINE (decl) << endl;

  type_template* t_node (0);

  if (tc == RECORD_TYPE)
    t_node = &emit_class_template (decl);
  else
    t_node = &emit_union_template (decl);

  if (COMPLETE_TYPE_P (t))
    unit_->new_edge<defines> (*scope_, *t_node, name);
  else
    unit_->new_edge<declares> (*scope_, *t_node, name);

  if (trace)
    ts << "end " <<  gcc_tree_code_name(tc) << " template " << name
       << " (" << t_node << ") at "
       << DECL_SOURCE_FILE (decl) << ":"
       << DECL_SOURCE_LINE (decl) << endl;
}

class_template& parser::impl::
emit_class_template (tree t, bool stub)
{
  // See if there is a stub already for this template.
  //
  class_template* ct_node (0);
  tree c (TREE_TYPE (DECL_TEMPLATE_RESULT (t)));

  if (node* n = unit_->find (t))
  {
    ct_node = &dynamic_cast<class_template&> (*n);
  }
  else
  {
    path f (DECL_SOURCE_FILE (t));
    size_t ln (DECL_SOURCE_LINE (t));
    size_t cl (DECL_SOURCE_COLUMN (t));

    ct_node = &unit_->new_node<class_template> (f, ln, cl, c);
    unit_->insert (t, *ct_node);
  }

  if (stub || !COMPLETE_TYPE_P (c))
    return *ct_node;

  class_scopes_.push_back (ct_node);

  // Collect member declarations so that we can traverse them in
  // the source code order. For now we are only interested in
  // nested class template declarations.
  //
  decl_set decls;

  for (tree d (TYPE_FIELDS (c)); d != NULL_TREE ; d = TREE_CHAIN (d))
  {
    switch (TREE_CODE (d))
    {
    case TEMPLATE_DECL:
      {
        if (DECL_CLASS_TEMPLATE_P (d))
          decls.insert (d);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  scope* prev_scope (scope_);
  scope_ = ct_node;

  for (decl_set::const_iterator i (decls.begin ()), e (decls.end ());
       i != e; ++i)
  {
    // Skip pragmas.
    //
    if (i->prag)
      continue;

    tree d (i->decl);

    switch (TREE_CODE (d))
    {
    case TEMPLATE_DECL:
      {
        emit_template_decl (d);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  // Diagnose any position pragmas that haven't been associated.
  //
  diagnose_unassoc_pragmas (decls);

  scope_ = prev_scope;
  class_scopes_.pop_back ();
  return *ct_node;
}

union_template& parser::impl::
emit_union_template (tree t, bool stub)
{
  // See if there is a stub already for this template.
  //
  union_template* ut_node (0);
  tree u (TREE_TYPE (DECL_TEMPLATE_RESULT (t)));

  if (node* n = unit_->find (t))
  {
    ut_node = &dynamic_cast<union_template&> (*n);
  }
  else
  {
    path f (DECL_SOURCE_FILE (t));
    size_t l (DECL_SOURCE_LINE (t));
    size_t c (DECL_SOURCE_COLUMN (t));

    ut_node = &unit_->new_node<union_template> (f, l, c, u);
    unit_->insert (t, *ut_node);
  }

  if (stub || !COMPLETE_TYPE_P (u))
    return *ut_node;

  class_scopes_.push_back (ut_node);

  // Collect member declarations so that we can traverse them in
  // the source code order. For now we are only interested in
  // nested class template declarations.
  //
  decl_set decls;

  for (tree d (TYPE_FIELDS (u)); d != NULL_TREE ; d = TREE_CHAIN (d))
  {
    switch (TREE_CODE (d))
    {
    case TEMPLATE_DECL:
      {
        if (DECL_CLASS_TEMPLATE_P (d))
          decls.insert (d);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  scope* prev_scope (scope_);
  scope_ = ut_node;

  for (decl_set::const_iterator i (decls.begin ()), e (decls.end ());
       i != e; ++i)
  {
    // Skip pragmas.
    //
    if (i->prag)
      continue;

    tree d (i->decl);

    switch (TREE_CODE (d))
    {
    case TEMPLATE_DECL:
      {
        emit_template_decl (d);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  // Diagnose any position pragmas that haven't been associated.
  //
  diagnose_unassoc_pragmas (decls);

  scope_ = prev_scope;
  class_scopes_.pop_back ();
  return *ut_node;
}

enum_& parser::impl::
emit_enum (tree e,
           access access,
           path const& file,
           size_t line,
           size_t clmn,
           bool stub)
{
  e = TYPE_MAIN_VARIANT (e);

  // See if there is a stub already for this type.
  //
  enum_* e_node (0);

  if (node* n = unit_->find (e))
    e_node = &dynamic_cast<enum_&> (*n);
  else
  {
    e_node = &unit_->new_node<enum_> (file, line, clmn, e);

    // Set the underlying type even for incomplete (forward-declared) enums.
    //
    tree ut (ENUM_UNDERLYING_TYPE (e));
    names* hint (unit_->find_hint (ut));
    integral_type* un = dynamic_cast<integral_type*> (
      unit_->find (TYPE_MAIN_VARIANT (ut)));

    // For "old" enums GCC creates a distinct type node and the only way to
    // get to one of the known integrals is via its name.
    //
    if (un == 0)
    {
      ut =  TREE_TYPE (TYPE_NAME (ut));
      un = dynamic_cast<integral_type*> (unit_->find (TYPE_MAIN_VARIANT (ut)));
    }

    underlies& edge (unit_->new_edge<underlies> (*un, *e_node));

    if (hint != 0)
      edge.hint (*hint);

    unit_->insert (e, *e_node);
  }

  if (stub || !COMPLETE_TYPE_P (e))
    return *e_node;

  // Traverse enumerators.
  //
  for (tree er (TYPE_VALUES (e)); er != NULL_TREE ; er = TREE_CHAIN (er))
  {
    char const* name (IDENTIFIER_POINTER (TREE_PURPOSE (er)));
    tree decl (TREE_VALUE (er));
    tree tval (DECL_INITIAL (decl));

    unsigned long long val (integer_value (tval));

    // There doesn't seem to be a way to get the proper position for
    // each enumerator.
    //
    enumerator& er_node = unit_->new_node<enumerator> (
      file, line, clmn, er, val);
    unit_->new_edge<enumerates> (*e_node, er_node);
    unit_->insert (decl, er_node);

    // In C++11 the enumerators are always available in the enum's
    // scope, even for old enums.
    //
    if (ops_.std () >= cxx_version::cxx11)
      unit_->new_edge<names> (*e_node, er_node, name, access::public_);

    // Inject enumerators into the outer scope unless this is an
    // enum class.
    //
    if (UNSCOPED_ENUM_P (e))
      unit_->new_edge<names> (*scope_, er_node, name, access);

    if (trace)
      ts << "\tenumerator " << name << " at " << file << ":" << line << endl;
  }

  return *e_node;
}

type& parser::impl::
emit_type (tree t,
           access access,
           path const& file,
           size_t line,
           size_t clmn)
{
  tree mv (TYPE_MAIN_VARIANT (t));

  if (trace)
  {
    ts << gcc_tree_code_name(TREE_CODE (t)) << " " << t
       << " main " << mv << endl;

    for (tree v (TYPE_MAIN_VARIANT (t)); v != 0; v = TYPE_NEXT_VARIANT (v))
      ts << "\tvariant " << v << " " << CP_TYPE_CONST_P (v) << endl;
  }

  node* n (unit_->find (mv));

  type& r (n != 0
           ? dynamic_cast<type&> (*n)
           : create_type (t, access, file, line, clmn));

  if (trace && n != 0)
    ts << "found node " << &r << " for type " << mv << endl;

  if (cp_type_quals (t) == TYPE_UNQUALIFIED)
  {
    unit_->insert (t, r); // Add this variant to the map.
    return r;
  }

  // See if this type already has this variant.
  //
  bool qc (CP_TYPE_CONST_P (t));
  bool qv (CP_TYPE_VOLATILE_P (t));
  bool qr (CP_TYPE_RESTRICT_P (t));

  for (type::qualified_iterator i (r.qualified_begin ());
       i != r.qualified_end (); ++i)
  {
    qualifier& q (i->qualifier ());

    if (q.const_ () == qc && q.volatile_ () == qv && q.restrict_ () == qr)
    {
      if (trace)
        ts << "found qualifier variant " << &q << endl;

      unit_->insert (t, q); // Add this variant to the map.
      return q;
    }
  }

  // No such variant yet. Create a new one. Qualified types are not
  // unique in the tree so don't add this node to the map.
  //
  qualifier& q (unit_->new_node<qualifier> (file, line, clmn, t, qc, qv, qr));
  qualifies& e (unit_->new_edge<qualifies> (q, r));
  unit_->insert (t, q);

  // See if there is a name hint for this type.
  //
  // If TREE_TYPE (TYPE_NAME (t)) != t then we have an inline qualifier,
  // as in:
  //
  // const foo x;
  //
  // If they are equal, then there are two possible cases. The first is
  // when we have a qualifier typedef, as in:
  //
  // typedef const foo cfoo;
  // cfoo x;
  //
  // The second is when we have a qualifier typedef but what is actually
  // used in the declaration is an inline qualifier, as in:
  //
  // typedef const foo cfoo;
  // const foo x;
  //
  // Unfortunately, in GCC, these two cases are indistinguishable. In
  // certain cases this can lead to a wrong hint being used for the base
  // type, for example:
  //
  // typedef foo my_foo;
  // typedef foo foo_t;
  // typedef const foo_t cfoo;
  //
  // const my_foo x;
  //
  // Above, the hint will be foo_t while it should be my_foo.
  //
  tree bt (0);

  if (tree decl = TYPE_NAME (t))
  {
    bt = TREE_TYPE (decl);

    if (t == bt)
    {
      // A const type can be named only with a typedef. Get the
      // original type.
      //
      tree ot (DECL_ORIGINAL_TYPE (decl));

      // And chase it one more time to get rid of the qualification.
      //
      decl = TYPE_NAME (ot);
      bt = decl != 0 ? TREE_TYPE (decl) : 0;
    }
  }

  if (bt != 0)
  {
    if (names* hint = unit_->find_hint (bt))
      e.hint (*hint);
  }

  process_named_pragmas (t, q);

  return q;
}

type& parser::impl::
create_type (tree t,
             access access,
             path const& file,
             size_t line,
             size_t clmn)
{
  type* r (0);
  gcc_tree_code_type tc (TREE_CODE (t));

  switch (tc)
  {
    //
    // User-defined types.
    //
  case RECORD_TYPE:
  case UNION_TYPE:
    {
      t = TYPE_MAIN_VARIANT (t);
      tree ti (TYPE_TEMPLATE_INFO (t));

      if (ti == NULL_TREE)
      {
        // Ordinary class. There are two situations which can lead
        // here. First is when we have an anonymous class that is
        // part of the declaration, for example:
        //
        // typedef const struct {...} s;
        //
        // The second situation is a named class definition which
        // we haven't parsed yet. In this case we are going to
        // create a "stub" class node which will be processed and
        // filled in later.
        //

        // Pointers to member functions are represented as record
        // types. Detect and handle this case.
        //
        if (TYPE_PTRMEMFUNC_P (t))
        {
          r = &unit_->new_node<unsupported_type> (
            file, line, clmn, t, "pointer_to_member_function_type");
          unit_->insert (t, *r);
        }
        else
        {
          tree d (TYPE_NAME (t));

          if (trace)
            ts << "start anon/stub " << gcc_tree_code_name(tc) << " at "
               << file << ":" << line << endl;

          if (d == NULL_TREE || IDENTIFIER_ANON_P (DECL_NAME (d)))
          {
            if (tc == RECORD_TYPE)
              r = &emit_class<class_> (t, file, line, clmn);
            else
              r = &emit_union<union_> (t, file, line, clmn);
          }
          else
          {
            // Use the "defining" declaration's file, line, and column
            // information to create the stub.
            //
            path f (DECL_SOURCE_FILE (d));
            size_t l (DECL_SOURCE_LINE (d));
            size_t c (DECL_SOURCE_COLUMN (d));

            if (tc == RECORD_TYPE)
              r = &emit_class<class_> (t, f, l, c, true);
            else
              r = &emit_union<union_> (t, f, l, c, true);
          }

          if (trace)
            ts << "end anon/stub " << gcc_tree_code_name(tc) << " (" << r << ")"
               << " at " << file << ":" << line << endl;
        }
      }
      else
      {
        // Template instantiation.
        //
        tree decl (TI_TEMPLATE (ti)); // DECL_TEMPLATE

        // Get to the most general template declaration.
        //
        while (DECL_TEMPLATE_INFO (decl))
          decl = DECL_TI_TEMPLATE (decl);

        type_template* t_node (0);

        // Find the template node or create a stub if none exist.
        //
        if (node* n = unit_->find (decl))
          t_node = &dynamic_cast<type_template&> (*n);
        else
        {
          if (trace)
            ts << "start stub " << gcc_tree_code_name(tc) << " template for ("
               << decl << ") at " << file << ":" << line << endl;

          if (tc == RECORD_TYPE)
            t_node = &emit_class_template (decl, true);
          else
            t_node = &emit_union_template (decl, true);

          if (trace)
            ts << "end stub " << gcc_tree_code_name(tc) << " template ("
               << t_node << ") at " << file << ":" << line << endl;
        }

        if (trace)
          ts << "start " << gcc_tree_code_name(tc) << " instantiation ("
             << t << ") for template (" << t_node << ")"
             << " at " << file << ":" << line << endl;

        type_instantiation* i_node (0);

        if (tc == RECORD_TYPE)
          i_node = &emit_class<class_instantiation> (t, file, line, clmn);
        else
          i_node = &emit_union<union_instantiation> (t, file, line, clmn);

        if (trace)
          ts << "end " << gcc_tree_code_name(tc) << " instantiation ("
             << static_cast<type*> (i_node) << ")"
             << " at " << file << ":" << line << endl;

        unit_->new_edge<instantiates> (*i_node, *t_node);
        process_named_pragmas (t, *i_node);
        r = i_node;
      }

      break;
    }
  case ENUMERAL_TYPE:
    {
      // The same logic as in the "ordinary class" case above
      // applies here.
      //

      t = TYPE_MAIN_VARIANT (t);
      tree d (TYPE_NAME (t));

      if (trace)
        ts << "start anon/stub " << gcc_tree_code_name(tc) << " at "
           << file << ":" << line << endl;

      if (d == NULL_TREE || IDENTIFIER_ANON_P (DECL_NAME (d)))
      {
        r = &emit_enum (t, access, file, line, clmn);
      }
      else
      {
        // Use the "defining" declaration's file, line, and column
        // information to create the stub.
        //
        path f (DECL_SOURCE_FILE (d));
        size_t l (DECL_SOURCE_LINE (d));
        size_t c (DECL_SOURCE_COLUMN (d));

        r = &emit_enum (t, access, f, l, c, true);
      }

      if (trace)
        ts << "end anon/stub " << gcc_tree_code_name(tc) << " (" << r << ")"
           << " at " << file << ":" << line << endl;

      break;
    }

    //
    // Derived types.
    //

  case ARRAY_TYPE:
    {
      unsigned long long size (0);

      if (tree index = TYPE_DOMAIN (t))
      {
        tree max (TYPE_MAX_VALUE (index));

        if (TREE_CODE (max) == INTEGER_CST)
        {
          size = integer_value (max);

          // The docs say that TYPE_DOMAIN will be NULL if the
          // array doesn't specify bounds. In reality, it is
          // set to ~0.
          //
          if (size == ~(unsigned long long) (0))
            size = 0;

          size++; // Convert max index to size.
        }
        else
        {
          error (file, line, clmn)
            << "non-integer array index " <<
            gcc_tree_code_name(TREE_CODE (max)) << endl;

          throw failed ();
        }
      }

      // In GCC tree a const array has both the array type itself and the
      // element type marked as const. This doesn't bode well with our
      // semantic graph model where we have a separate type node for
      // qualifiers. To fix this, we are going to strip the const
      // qualification from the element type and only preserve it in
      // the array type. In other words, we view it as "constant array"
      // rather than "array of constant elements".
      //
      using semantics::array; // vs std::array.

      tree bt (TREE_TYPE (t));
      tree bt_mv (TYPE_MAIN_VARIANT (bt));
      type& bt_node (emit_type (bt_mv, access::public_, file, line, clmn));
      t = TYPE_MAIN_VARIANT (t);
      array& a (unit_->new_node<array> (file, line, clmn, t, size));
      unit_->insert (t, a);
      contains& edge (unit_->new_edge<contains> (a, bt_node));

      // See if there is a name hint for the base type.
      //
      if (names* hint = unit_->find_hint (
            cp_type_quals (bt) == TYPE_UNQUALIFIED ? bt : bt_mv))
        edge.hint (*hint);

      process_named_pragmas (t, a);
      r = &a;
      break;
    }
  case REFERENCE_TYPE:
    {
      tree bt (TREE_TYPE (t));
      type& bt_node (emit_type (bt, access::public_, file, line, clmn));
      t = TYPE_MAIN_VARIANT (t);
      reference& ref (unit_->new_node<reference> (file, line, clmn, t));
      unit_->insert (t, ref);
      references& edge (unit_->new_edge<references> (ref, bt_node));

      // See if there is a name hint for the base type.
      //
      if (names* hint = unit_->find_hint (bt))
        edge.hint (*hint);

      process_named_pragmas (t, ref);
      r = &ref;
      break;
    }
  case POINTER_TYPE:
    {
      if (!TYPE_PTRMEM_P (t))
      {
        tree bt (TREE_TYPE (t));
        type& bt_node (emit_type (bt, access::public_, file, line, clmn));
        t = TYPE_MAIN_VARIANT (t);
        pointer& p (unit_->new_node<pointer> (file, line, clmn, t));
        unit_->insert (t, p);
        points& edge (unit_->new_edge<points> (p, bt_node));

        // See if there is a name hint for the base type.
        //
        if (names* hint = unit_->find_hint (bt))
          edge.hint (*hint);

        process_named_pragmas (t, p);
        r = &p;
      }
      else
      {
        t = TYPE_MAIN_VARIANT (t);
        r = &unit_->new_node<unsupported_type> (
          file, line, clmn, t, "pointer_to_data_member_type");
        unit_->insert (t, *r);

        if (trace)
          ts << "unsupported pointer_to_data_member_type (" << r << ")"
             << " at " << file << ":" << line << endl;
      }
      break;
    }
  default:
    {
      t = TYPE_MAIN_VARIANT (t);
      r = &unit_->new_node<unsupported_type> (
        file, line, clmn, t, gcc_tree_code_name(tc));
      unit_->insert (t, *r);

      if (trace)
        ts << "unsupported " << gcc_tree_code_name(tc) << " (" << r << ")"
           << " at " << file << ":" << line << endl;

      break;
    }
  }

  return *r;
}

string parser::impl::
emit_type_name (tree type, bool direct)
{
  // First see if there is a "direct" name for this type.
  //
  if (direct)
  {
    if (tree decl = TYPE_NAME (type))
    {
      tree t (TREE_TYPE (decl));

      if (t != 0 && same_type_p (type, t))
        return IDENTIFIER_POINTER (DECL_NAME (decl));
    }
  }

  string r;

  if (CP_TYPE_CONST_P (type))
    r += " const";

  if (CP_TYPE_VOLATILE_P (type))
    r += " volatile";

  if (CP_TYPE_RESTRICT_P (type))
    r += " __restrict";

  gcc_tree_code_type tc (TREE_CODE (type));

  switch (tc)
  {
    //
    // User-defined types.
    //

  case RECORD_TYPE:
  case UNION_TYPE:
    {
      tree ti (TYPE_TEMPLATE_INFO (type));

      if (ti == NULL_TREE)
      {
        // Ordinary class.
        //
        type = TYPE_MAIN_VARIANT (type);

        // Pointers to member functions are represented as record
        // types and don't have names, not even the synthesized ones.
        //
        if (TYPE_PTRMEMFUNC_P (type))
          r = "<pointer-to-member-function>" + r;
        else
        {
          tree name (TYPE_NAME (type));
          r = IDENTIFIER_POINTER (DECL_NAME (name)) + r;
        }
      }
      else
      {
        // Template instantiation.
        //
        tree decl (TI_TEMPLATE (ti)); // DECL_TEMPLATE
        string id (IDENTIFIER_POINTER (DECL_NAME (decl)));

        id += '<';

        tree args (INNERMOST_TEMPLATE_ARGS (TI_ARGS (ti)));

        for (size_t i (0), n (TREE_VEC_LENGTH (args)); i < n ; ++i)
        {
          tree a (TREE_VEC_ELT (args, i));

          if (i != 0)
            id += ", ";

          // Assume integer and type-only arguments.
          //
          if (TREE_CODE (a) == INTEGER_CST)
            id += to_string (integer_value (a));
          else
            id += emit_type_name (a);
        }

        id += '>';

        r = id + r;
      }

      break;
    }

  case ENUMERAL_TYPE:
    {
      type = TYPE_MAIN_VARIANT (type);
      tree decl (TYPE_NAME (type));
      r = IDENTIFIER_POINTER (DECL_NAME (decl)) + r;
      break;
    }

    //
    // Derived types.
    //

  case ARRAY_TYPE:
    {
      unsigned long long size (0);

      if (tree index = TYPE_DOMAIN (type))
      {
        tree max (TYPE_MAX_VALUE (index));

        if (TREE_CODE (max) == INTEGER_CST)
        {
          size = integer_value (max);

          // Same as above.
          //
          if (size == ~(unsigned long long) (0))
            size = 0;

          size++;
        }
        else
        {
          // Non-integer array index which we do not support. The
          // error has been/will be issued in emit_type.
          //
        }
      }

      tree t (TREE_TYPE (type));

      if (size != 0)
      {
        ostringstream ostr;
        ostr << size;
        r = emit_type_name (t) + "[" + ostr.str () + "]" + r;
      }
      else
        r = emit_type_name (t) + "[]" + r;

      break;
    }
  case REFERENCE_TYPE:
    {
      tree t (TREE_TYPE (type));
      r = emit_type_name (t) + "&" + r;
      break;
    }
  case POINTER_TYPE:
    {
      if (!TYPE_PTRMEM_P (type))
      {
        tree t (TREE_TYPE (type));
        r = emit_type_name (t) + "*" + r;
      }
      else
        r = "<pointer_to_member_type>";

      break;
    }

    //
    // Fundamental types.
    //

  case VOID_TYPE:
  case REAL_TYPE:
  case BOOLEAN_TYPE:
  case INTEGER_TYPE:
    {
      type = TYPE_MAIN_VARIANT (type);
      tree decl (TYPE_NAME (type));
      r = IDENTIFIER_POINTER (DECL_NAME (decl)) + r;
      break;
    }
  default:
    {
      r = "<" + string (gcc_tree_code_name(tc)) + ">";
      break;
    }
  }

  return r;
}

void parser::impl::
process_pragmas (declaration const& decl,
                 node& node,
                 string const& name,
                 decl_set::const_iterator begin,
                 decl_set::const_iterator cur,
                 decl_set::const_iterator /*end*/)
{
  // First process the position pragmas by iterating backwards until
  // we get to the preceding non-pragma declaration that has been
  // associated.
  //
  pragma_set prags;

  if (cur != begin)
  {
    decl_set::const_iterator i (cur);
    for (--i; i != begin && (i->prag != 0 || !i->assoc); --i) ;

    for (; i != cur; ++i)
    {
      if (i->prag == 0) // Skip declarations.
        continue;

      assert (!i->assoc);

      if (i->prag->check (decl, name, i->prag->pragma_name, i->prag->loc))
        prags.insert (*i->prag);
      else
        error_++; // Diagnostic has already been issued.

      i->assoc = true; // Mark this pragma as associated.
    }

    cur->assoc = true; // Mark the declaration as associated.
  }

  // Now see if there are any named pragmas for this declaration. By
  // doing this after handling the position pragmas we ensure correct
  // overriding.
  //
  {
    decl_pragmas::const_iterator i (decl_pragmas_.find (decl));

    if (i != decl_pragmas_.end ())
      prags.insert (i->second.begin (), i->second.end ());
  }

  // Finally, copy the resulting pragma set to context.
  //
  for (pragma_set::iterator i (prags.begin ()); i != prags.end (); ++i)
    add_pragma (node, i->second);
}

void parser::impl::
process_named_pragmas (declaration const& decl, node& node)
{
  pragma_set prags;

  decl_pragmas::const_iterator i (decl_pragmas_.find (decl));

  if (i != decl_pragmas_.end ())
    prags.insert (i->second.begin (), i->second.end ());

  // Copy the resulting pragma set to context.
  //
  for (pragma_set::iterator i (prags.begin ()); i != prags.end (); ++i)
    add_pragma (node, i->second);
}

void parser::impl::
add_pragma (node& n, pragma const& p)
{
  if (trace)
    ts << "\t\t pragma " << p.pragma_name << endl;

  string const& k (p.context_name);

  if (p.add == 0)
  {
    n.set (k, p.value);
    n.set (k + "-location", p.loc);
  }
  else
    p.add (n, k, p.value, p.loc);
}

void parser::impl::
diagnose_unassoc_pragmas (decl_set const& decls)
{
  for (decl_set::const_iterator i (decls.begin ()), e (decls.end ());
       i != e; ++i)
  {
    if (i->prag && !i->assoc)
    {
      pragma const& p (*i->prag);
      error (p.loc)
        << "db pragma '" << p.pragma_name << "' is not associated with a "
        << "declaration" << endl;
      error_++;
    }
  }
}

string parser::impl::
fq_scope (tree decl)
{
  string s, tmp;

  for (tree scope (CP_DECL_CONTEXT (decl));
       scope != global_namespace;)
  {
    tree prev (CP_DECL_CONTEXT (scope));

    // If this is an inline namespace, pretend it doesn't exist.
    //
#if BUILDING_GCC_MAJOR >= 8
    if (!is_nested_namespace (prev, scope, true))
#else
    if (!is_associated_namespace (prev, scope))
#endif
    {
      tree n = DECL_NAME (scope);

      tmp = "::";
      tmp += (n != NULL_TREE ? IDENTIFIER_POINTER (n) : "");
      tmp += s;
      s.swap (tmp);
    }

    scope = prev;
  }

  return s;
}

//
// parser
//

parser::
~parser ()
{
  // Needs parser::impl definition.
}

parser::
parser (options const& ops,
        loc_pragmas& lp,
        ns_loc_pragmas& nslp,
        decl_pragmas& dp)
    : impl_ (new impl (ops, lp, nslp, dp))
{
}

unique_ptr<unit> parser::
parse (tree global_scope, path const& main_file)
{
  return impl_->parse (global_scope, main_file);
}
