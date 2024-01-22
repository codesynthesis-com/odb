// file      : odb/context.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_CONTEXT_HXX
#define ODB_CONTEXT_HXX

#include <odb/gcc-fwd.hxx>

#include <map>
#include <set>
#include <list>
#include <stack>
#include <vector>
#include <string>
#include <memory>  // std::unique_ptr
#include <ostream>
#include <cstddef> // std::size_t
#include <iostream>

#include <libcutl/re.hxx>
#include <libcutl/shared-ptr.hxx>

#include <odb/options.hxx>
#include <odb/features.hxx>
#include <odb/location.hxx>
#include <odb/cxx-token.hxx>
#include <odb/semantics.hxx>
#include <odb/semantics/relational/name.hxx>
#include <odb/semantics/relational/model.hxx>
#include <odb/traversal.hxx>

using std::endl;
using std::cerr;

// Regex.
//
using cutl::re::regex;
using cutl::re::regexsub;
typedef cutl::re::format regex_format;

typedef std::vector<regexsub> regex_mapping;

// Forward-declarations.
//
class cxx_string_lexer;

// Generic exception thrown to indicate a failure when diagnostics
// has already been issued (to stderr).
//
class operation_failed {};

// Keep this enum synchronized with the one in libodb/odb/pointer-traits.hxx.
//
enum pointer_kind
{
  pk_raw,
  pk_unique,
  pk_shared,
  pk_weak
};

// Keep this enum synchronized with the one in libodb/odb/container-traits.hxx.
//
enum container_kind
{
  ck_ordered,
  ck_set,
  ck_multiset,
  ck_map,
  ck_multimap
};

// The same as class_kind in libodb/odb/traits.hxx.
//
enum class_kind
{
  class_object,
  class_view,
  class_composite,
  class_other
};

// Data member path.
//
// If it is a direct member of an object, then we will have just
// one member. However, if this is a member inside a composite
// value, then we will have a "path" constructed out of members
// that lead all the way from the object member to the innermost
// composite value member.
//
struct data_member_path: std::vector<semantics::data_member*>
{
  data_member_path () {}

  explicit
  data_member_path (semantics::data_member& m) {push_back (&m);}

  // Return true if this is a sub-path of (or equal to) the
  // specified path.
  //
  bool
  sub (const data_member_path& p) const
  {
    size_t n (p.size ());

    if (n > size ())
      return false;

    for (size_t i (0); i != n; ++i)
      if ((*this)[i] != p[i])
        return false;

    return true;
  }
};

// Class inheritance chain, from the most derived to base.
//
typedef std::vector<semantics::class_*> class_inheritance_chain;

// A list of inheritance chains for a data member in an object.
// The first entry in this list would correspond to the object.
// All subsequent entries, if any, correspond to composite
// values.
//
typedef std::vector<class_inheritance_chain> data_member_scope;

//
// Semantic graph context types.
//

// Custom C++ type mapping.
//
struct custom_cxx_type
{
  custom_cxx_type (): type_node (0), as_node (0) {}

  std::string
  translate_to (std::string const& v) const {return translate (v, to);}

  std::string
  translate_from (std::string const& v) const {return translate (v, from);}

  tree type_node;
  std::string type_name;
  semantics::type* type;
  semantics::names* type_hint;

  tree as_node;
  std::string as_name;
  semantics::type* as;
  semantics::names* as_hint;

  cxx_tokens to;
  bool to_move; // Single (?), so can move.

  cxx_tokens from;
  bool from_move; // Single (?), so can move.

  location_t loc;
  tree scope; // Scope for which this mapping is defined.

private:
  static std::string
  translate (std::string const&, const cxx_tokens&);
};

typedef std::vector<custom_cxx_type> custom_cxx_types;
typedef std::map<semantics::type*, custom_cxx_type*> custom_cxx_type_map;


// Object or view pointer.
//
struct class_pointer
{
  std::string name;
  tree scope;
  location_t loc;
};

//
//
struct default_value
{
  enum kind_type
  {
    reset,   // Default value reset.
    null,
    boolean,  // Literal contains value (true or false).
    integer,  // Integer number. Literal contains sign.
    floating, // Floating-point number.
    string,   // Literal contains value.
    enumerator // Literal is the name, enum_value is the tree node.
  };

  kind_type kind;
  std::string literal;

  union
  {
    tree enum_value;
    unsigned long long int_value;
    double float_value;
  };
};

// Database potentially-qualified and unqualifed names.
//
using semantics::relational::qname;
using semantics::relational::uname;

// Object or table associated with the view.
//
struct view_object
{
  // Return a diagnostic name for this association. It is either the
  // alias, unqualified object name, or string representation of the
  // table name.
  //
  std::string
  name () const;

  enum kind_type { object, table };
  enum join_type { left, right, full, inner, cross };

  kind_type kind;
  join_type join;
  tree obj_node;         // Tree node if kind is object.
  std::string obj_name;  // Name as specified in the pragma if kind is object.
  qname tbl_name;        // Table name if kind is table.
  std::string alias;
  tree scope;
  location_t loc;
  semantics::class_* obj;
  semantics::data_member* ptr; // Corresponding object pointer, if any.

  cxx_tokens cond; // Join condition tokens.
};

typedef std::vector<view_object> view_objects;

// The view_alias_map does not contain entries for tables.
//
typedef std::map<std::string, view_object*> view_alias_map;
typedef std::map<semantics::class_*, view_object*> view_object_map;

// Collection of relationships via which the objects are joined.
// We need this information to figure out which alias/table
// names to use for columns corresponding to inverse object
// pointers inside objects that this view may be loading.
//
// The first object is the pointer (i.e., the one containing
// this data member) while the other is the pointee. In other
// words, first -> second. We always store the direct (i.e.,
// non-inverse) side of the relationship. Note also that there
// could be multiple objects joined using the same relationship.
//
typedef
std::multimap<data_member_path, std::pair<view_object*, view_object*> >
view_relationship_map;

//
//
struct view_query
{
  view_query (): distinct (false), for_update (false) {}

  enum kind_type
  {
    runtime,
    complete_select,  // SELECT query.
    complete_execute, // Stored procedure call.
    condition
  };

  kind_type kind;
  std::string literal;
  cxx_tokens expr;
  tree scope;
  location_t loc;

  // Result modifiers (only for condition).
  //
  bool distinct;   // SELECT DISTINCT
  bool for_update; // SELECT FOR UPDATE
};

//
//
struct table_column
{
  qname table;
  std::string column;
  bool expr; // True if column is an expression, and therefore should not
             // be quoted.

  table_column () {}

  explicit
  table_column (const std::string& c): column (c), expr (false) {}
};

//
//
struct column_expr_part
{
  enum kind_type
  {
    literal,
    reference
  };

  kind_type kind;
  std::string value;
  qname table;                  // Table name/alias for references.
  data_member_path member_path; // Path to member for references.

  // Scope and location of this pragma. Used to resolve the member name.
  //
  tree scope;
  location_t loc;
};

struct column_expr: std::vector<column_expr_part>
{
  location_t loc;
};

//
//
struct member_access
{
  member_access (const location& l, const char* k, bool s)
      : loc (l), kind (k), synthesized (s), by_value (false) {}

  // Return true of we have the (?) placeholder.
  //
  bool
  placeholder () const;

  // Return true if this is a synthesized expression that goes
  // directly for the member.
  //
  bool
  direct () const
  {
    return synthesized && expr.size () == 3; // this.member
  }

  bool
  empty () const
  {
    return expr.empty ();
  }

  // Issues diagnostics and throws operation_failed if expression is
  // empty.
  //
  std::string
  translate (std::string const& obj,
             std::string const& val = std::string (),
             std::string const& db = std::string ()) const;

  location loc;
  const char* kind; // accessor/modifier; used for diagnostics.
  bool synthesized; // If true, then this is a synthesized expression.
  cxx_tokens expr;
  bool by_value;   // True if accessor returns by value. False doesn't
                   // necessarily mean that it is by reference.
};

//
//
struct model_version
{
  unsigned long long base;
  unsigned long long current;
  bool open;
};

// Sections.
//
struct object_section
{
  virtual bool
  compare (object_section const&) const = 0;

  virtual bool
  separate_load () const = 0;

  virtual bool
  separate_update () const = 0;
};

inline bool
operator== (object_section const& x, object_section const& y)
{
  return x.compare (y);
}

inline bool
operator!= (object_section const& x, object_section const& y)
{
  return !x.compare (y);
}

// Main section.
//
struct main_section_type: object_section
{
  virtual bool
  compare (object_section const& s) const;

  virtual bool
  separate_load () const {return false;}

  virtual bool
  separate_update () const {return false;}
};

inline bool
operator== (main_section_type const&, main_section_type const&)
{
  return true; // There is only one main section.
}

extern main_section_type main_section;

// User-defined section.
//
struct user_section: object_section
{
  enum load_type
  {
    load_eager,
    load_lazy
  };

  enum update_type
  {
    update_always,
    update_change,
    update_manual
  };

  enum special_type
  {
    special_ordinary,
    special_version  // Fake section for optimistic version update.
  };

  user_section (semantics::data_member& m,
                semantics::class_& o,
                std::size_t i,
                load_type l,
                update_type u,
                special_type s = special_ordinary)
      : member (&m), object (&o), base (0), index (i),
        load (l), update (u), special (s),
        total (0), inverse (0), readonly (0), versioned (false),
        containers (false), readwrite_containers (false),
        versioned_containers (false), readwrite_versioned_containers (false) {}

  virtual bool
  compare (object_section const& s) const;

  virtual bool
  separate_load () const {return load != load_eager;}

  virtual bool
  separate_update () const
  {
    // A separately-loaded section is always separately-updated since
    // it might not be loaded when update is requested.
    //
    return separate_load () || update != update_always;
  }

  bool
  load_empty () const;

  bool
  update_empty () const;

  bool
  empty () const
  {
    return load_empty () && update_empty ();
  }

  // A section is optimistic if the object that contains it is optimistic.
  // For polymorphic hierarchies, only sections contained in the root are
  // considered optimistic.
  //
  bool
  optimistic () const;

  semantics::data_member* member; // Data member of this section.
  semantics::class_* object;      // Object containing this section.
  user_section* base;             // Base of this section.
  std::size_t index;              // Index of this sections.

  load_type load;
  update_type update;
  special_type special;

  // Column counts.
  //
  std::size_t total;
  std::size_t inverse;
  std::size_t readonly;

  bool versioned;

  bool containers;
  bool readwrite_containers;

  bool versioned_containers;
  bool readwrite_versioned_containers;

  // Total counts across overrides.
  //
  std::size_t
  total_total () const
  {
    user_section* b (total_base ());
    return total + (b == 0 ? 0 : b->total_total ());
  }

  std::size_t
  total_inverse () const
  {
    user_section* b (total_base ());
    return inverse + (b == 0 ? 0 : b->total_inverse ());
  }

  std::size_t
  total_readonly () const
  {
    user_section* b (total_base ());
    return readonly + (b == 0 ? 0 : b->total_readonly ());
  }

  bool
  total_containers ()
  {
    user_section* b (total_base ());
    return containers || (b != 0 && b->total_containers ());
  }

  bool
  total_readwrite_containers ()
  {
    user_section* b (total_base ());
    return readwrite_containers ||
      (b != 0 && b->total_readwrite_containers ());
  }

private:
  user_section*
  total_base () const;
};

inline bool
operator== (user_section const& x, user_section const& y)
{
  return x.member == y.member;
}

// Using list for pointer for element stability (see user_section::base).
//
struct user_sections: std::list<user_section>
{
  // Count sections that have something to load.
  //
  static unsigned short const count_load         = 0x01;

  // Count sections that are non-eager but have nothing to load.
  //
  static unsigned short const count_load_empty   = 0x02;

  // Count sections that have something to update.
  //
  static unsigned short const count_update       = 0x04;

  // Count sections that have nothing to update.
  //
  static unsigned short const count_update_empty = 0x08;

  // Count sections that are optimistic.
  //
  static unsigned short const count_optimistic = 0x10;

  // Modifiers:
  //

  // Don't exclude fake optimistic version update section from the count.
  //
  static unsigned short const count_special_version = 0x20;

  // Only count versioned sections.
  //
  static unsigned short const count_versioned_only = 0x40;


  // Count all sections, but excluding special.
  //
  static unsigned short const count_all = count_update       |
                                          count_update_empty;

  static unsigned short const count_new      = 0x1000;
  static unsigned short const count_override = 0x2000;
  static unsigned short const count_total    = 0x4000;

  std::size_t
  count (unsigned short flags) const;

  user_sections (semantics::class_& o): object (&o) {};
  semantics::class_* object;
};

// Context.
//
class context
{
public:
  typedef std::size_t size_t;
  typedef std::string string;
  typedef std::vector<string> strings;
  typedef std::ostream ostream;

  typedef ::options options_type;

  static string
  upcase (string const&);

public:
  // Return cvr-unqualified base of the type, or type itself, if it is
  // not qualified.
  //
  static semantics::type&
  utype (semantics::type&);

  // The same as above, but also returns the name hint for the unqualified
  // type. If the original type is already unqualified, then the hint
  // argument is not modified.
  //
  static semantics::type&
  utype (semantics::type&, semantics::names*& hint);

  // The same for a member's type but also do custom C++ type translation.
  //
  static semantics::type&
  utype (semantics::data_member& m, const custom_cxx_type** translation = 0)
  {
    semantics::names* hint (0);
    return utype (m, hint, string (), translation);
  }

  static semantics::type&
  utype (semantics::data_member& m,
         string const& key_prefix,
         const custom_cxx_type** translation = 0)
  {
    semantics::names* hint (0);
    return utype (m, hint, key_prefix, translation);
  }

  // In addition to the unqualified type, this version also returns the
  // name hint for this type. If the member type is already unqualified,
  // then the hint is from the belongs edge. Otherwise, it is from the
  // qualifies edge.
  //
  static semantics::type&
  utype (semantics::data_member&,
         semantics::names*& hint,
         string const& key_prefix = string (),
         const custom_cxx_type** translation = 0);

  static semantics::type&
  utype (const data_member_path& mp, const custom_cxx_type** translation = 0)
  {
    return utype (*mp.back (), translation);
  }

  static semantics::type&
  utype (const data_member_path& mp,
         string const& key_prefix,
         const custom_cxx_type** translation = 0)
  {
    return utype (*mp.back (), key_prefix, translation);
  }

  static semantics::type&
  utype (const data_member_path& mp,
         semantics::names*& hint,
         string const& key_prefix = string (),
         const custom_cxx_type** translation = 0)
  {
    return utype (*mp.back (), hint, key_prefix, translation);
  }

  // For arrays this function returns true if the (innermost) element
  // type is const.
  //
  static bool
  const_type (semantics::type&);

  static bool
  const_member (semantics::data_member& m) {return const_type (m.type ());}

  // Form a reference type for a not mapped, actual member type. If
  // make_const is true, then add top-level const qualifier, unless
  // it is already there. If it is false, then strip it if it is
  // already there. If var is not empty, then embed the variable
  // name into the type (e.g., char (*v)[3]). If decay_array is
  // false then don't decay the (top-level) array to a pointer.
  //
  static string
  member_ref_type (semantics::data_member& m,
                   bool make_const,
                   string const& var = "",
                   bool decay_array = true)
  {
    return type_ref_type (
      m.type (), m.belongs ().hint (), make_const, var, decay_array);
  }

  static string
  type_ref_type (semantics::type&,
                 semantics::names* hint,
                 bool make_const,
                 string const& var = "",
                 bool decay_array = true);

  // Form a value type for a not mapped, actual member type. If make_const
  // is true, then add top-level const qualifier, unless it is already
  // there. If it is false, then strip it if it is already there. If var is
  // not empty, then embed the variable name into the type (e.g., char v[3]).
  //
  static string
  member_val_type (semantics::data_member& m,
                   bool make_const,
                   string const& var = "")
  {
    return type_val_type (m.type (), m.belongs ().hint (), make_const, var);
  }

  static string
  type_val_type (semantics::type&,
                 semantics::names* hint,
                 bool make_const,
                 string const& var = "");

  // Member access helpers. Database can be empty. If type is not empty,
  // then it should be the non-cvr type of the member (e.g., id_type).
  //
  void
  set_member (semantics::data_member& m,
              const std::string& obj,
              const std::string& val,
              const std::string& db,
              const std::string& type = "");

  void
  inc_member (semantics::data_member& m,
              const std::string& sobj, // Set expression object.
              const std::string& gobj, // Get expression object.
              const std::string& type = "");

public:
  // Resolve data member name in the form "a.b.c" to the data member path,
  // issuing diagnostics and throwing operation_filed in case of an error.
  // This function stops if it encounters a container leaving lex usable
  // to continue parsing.
  //
  void
  resolve_data_members (data_member_path& append,
                        semantics::class_& scope,
                        const std::string& name,
                        const location&,
                        cxx_string_lexer&);

  data_member_path
  resolve_data_members (semantics::class_& scope,
                        const std::string& name,
                        const location& l,
                        cxx_string_lexer& lex)
  {
    data_member_path r;
    resolve_data_members (r, scope, name, l, lex);
    return r;
  }

  // Predicates.
  //
public:
  static bool
  object (semantics::type& t)
  {
    return t.count ("object");
  }

  static bool
  view (semantics::type& t)
  {
    return t.count ("view");
  }

  // Direct member of a view.
  //
  static bool
  view_member (semantics::data_member& m)
  {
    return view (dynamic_cast<semantics::class_&> (m.scope ()));
  }

  // Check whether the type is a wrapper. Return the wrapped type if
  // it is a wrapper and NULL otherwise. Note that the returned type
  // may be cvr-qualified.
  //
  static semantics::type*
  wrapper (semantics::type& t)
  {
    return t.count ("wrapper") && t.get<bool> ("wrapper")
      ? t.get<semantics::type*> ("wrapper-type")
      : 0;
  }

  static semantics::type*
  wrapper (semantics::type& t, semantics::names*& hint)
  {
    if (t.count ("wrapper") && t.get<bool> ("wrapper"))
    {
      hint = t.get<semantics::names*> ("wrapper-hint");
      return t.get<semantics::type*> ("wrapper-type");
    }
    else
      return 0;
  }

  // Composite value type is a class type that was explicitly marked
  // as value type and there was no database type mapping provided for
  // it by the user (specifying the database type makes the value type
  // simple).
  //
  static bool
  composite (semantics::class_& c)
  {
    if (c.count ("composite-value"))
      return c.get<bool> ("composite-value");
    else
      return composite_ (c);
  }

  // Return the class object if this type is a composite value type
  // and NULL otherwise.
  //
  static semantics::class_*
  composite (semantics::type& t)
  {
    semantics::class_* c (dynamic_cast<semantics::class_*> (&t));
    return c != 0 && composite (*c) ? c : 0;
  }

  // As above but also "sees through" wrappers.
  //
  static semantics::class_*
  composite_wrapper (semantics::type& t)
  {
    if (semantics::class_* c = composite (t))
      return c;
    else if (semantics::type* wt = wrapper (t))
      return composite (utype (*wt));
    else
      return 0;
  }

  // Check if a data member is a container. "Sees through" wrappers and
  // returns the actual container type or NULL if not a container.
  //
  // We require data member as input instead of the type because the
  // same type (e.g., vector<char>) can be used for both container
  // and simple value members.
  //
  static semantics::type*
  container (semantics::data_member& m)
  {
    // The same type can be used as both a container and a simple value.
    //
    if (m.count ("simple"))
      return 0;

    semantics::type* t (&utype (m));

    if (semantics::type* wt = wrapper (*t))
      t = &utype (*wt);

    return t->count ("container-kind") ? t : 0;
  }

  static semantics::class_*
  object_pointer (semantics::type& t)
  {
    return t.get<semantics::class_*> ("element-type", 0);
  }

  // If this data member is or is part of an object pointer, then
  // return the member that is the pointer. Otherwise, return 0.
  //
  static semantics::data_member*
  object_pointer (data_member_path const&);

  static semantics::class_*
  points_to (semantics::data_member& m)
  {
    return m.get<semantics::class_*> ("points-to", 0);
  }

  static bool
  abstract (semantics::class_& c)
  {
    // If a class is abstract in the C++ sense then it is also abstract in
    // the database sense.
    //
    return c.abstract () || c.count ("abstract");
  }

  static bool
  session (semantics::class_& c)
  {
    return c.get<bool> ("session");
  }

  static bool
  transient (semantics::data_member& m)
  {
    return m.count ("transient");
  }

  // Return the deletion version or 0 if not soft-deleted.
  //
  static unsigned long long
  deleted (semantics::class_& c, location_t* l = 0)
  {
    unsigned long long v (c.get<unsigned long long> ("deleted", 0));

    if (v != 0 && l != 0)
      *l = c.get<location_t> ("deleted-location");

    return v;
  }

  static unsigned long long
  deleted (semantics::data_member& m, location_t* l = 0)
  {
    unsigned long long v (m.get<unsigned long long> ("deleted", 0));

    if (v != 0 && l != 0)
      *l = m.get<location_t> ("deleted-location");

    return v;
  }

  static unsigned long long
  deleted (data_member_path const& mp, location_t* l = 0)
  {
    unsigned long long r (0);

    // Find the earliest version since this member was deleted.
    //
    for (data_member_path::const_reverse_iterator i (mp.rbegin ());
         i != mp.rend (); ++i)
    {
      unsigned long long v ((*i)->get<unsigned long long> ("deleted", 0));
      if (v != 0 && (r == 0 || v < r))
      {
        r = v;

        if (l != 0)
          *l = (*i)->get<location_t> ("deleted-location");
      }
    }

    return r;
  }

  static semantics::data_member*
  deleted_member (data_member_path const& mp)
  {
    semantics::data_member* m (0);

    // Find the earliest version since this member was deleted.
    //
    unsigned long long r (0);
    for (data_member_path::const_reverse_iterator i (mp.rbegin ());
         i != mp.rend (); ++i)
    {
      unsigned long long v ((*i)->get<unsigned long long> ("deleted", 0));
      if (v != 0 && (r == 0 || v < r))
      {
        r = v;
        m = *i;
      }
    }

    return m;
  }

  // Return the addition version or 0 if not soft-added.
  //
  static unsigned long long
  added (semantics::class_& c) // Used for composite only.
  {
    return c.get<unsigned long long> ("added", 0);
  }

  static unsigned long long
  added (semantics::data_member& m)
  {
    return m.get<unsigned long long> ("added", 0);
  }

  static unsigned long long
  added (data_member_path const& mp)
  {
    unsigned long long r (0);

    // Find the latest version since this member was added.
    //
    for (data_member_path::const_reverse_iterator i (mp.rbegin ());
         i != mp.rend (); ++i)
    {
      unsigned long long v ((*i)->get<unsigned long long> ("added", 0));
      if (v != 0 && v > r)
        r = v;
    }

    return r;
  }

  static semantics::data_member*
  added_member (data_member_path const& mp)
  {
    semantics::data_member* m (0);

    // Find the latest version since this member was added.
    //
    unsigned long long r (0);
    for (data_member_path::const_reverse_iterator i (mp.rbegin ());
         i != mp.rend (); ++i)
    {
      unsigned long long v ((*i)->get<unsigned long long> ("added", 0));
      if (v != 0 && v > r)
      {
        r = v;
        m = *i;
      }
    }

    return m;
  }

  static bool
  id (semantics::data_member& m)
  {
    return m.count ("id");
  }

  // If this data member is or is part of an id member, then return
  // the member that is marked as the id. Otherwise, return 0.
  //
  static semantics::data_member*
  id (data_member_path const&);

  static bool
  auto_ (semantics::data_member& m)
  {
    return id (m) && m.count ("auto");
  }

  // Must be a path returned by id(). In other words, it assumes
  // the path is to the id member.
  //
  static bool
  auto_ (data_member_path& mp)
  {
    return mp.front ()->count ("auto");
  }

  // The member scope is used to override readonly status when a readonly
  // class (object or composite value) inherits from a readwrite base.
  //
  static bool
  readonly (data_member_path const&, data_member_scope const&);

  static bool
  readonly (semantics::data_member&);

  static bool
  readonly (semantics::class_& c)
  {
    return c.count ("readonly");
  }

  // Null-able.
  //
  bool
  null (data_member_path const&) const;

  bool
  null (semantics::data_member&) const;

  bool
  null (semantics::data_member&, string const& key_prefix) const;

  // Optimistic concurrency.
  //
  static semantics::data_member*
  optimistic (semantics::class_& c)
  {
    // Set by the validator.
    //
    return c.get<semantics::data_member*> ("optimistic-member", 0);
  }

  static bool
  version (semantics::data_member& m)
  {
    return m.count ("version");
  }

  static bool
  version (const data_member_path& mp)
  {
    return mp.size () == 1 && mp.back ()->count ("version");
  }

  // Polymorphic inheritance. Return root of the hierarchy or NULL if
  // not polymorphic.
  //
  static semantics::class_*
  polymorphic (semantics::class_& c)
  {
    // Set by the validator.
    //
    return c.get<semantics::class_*> ("polymorphic-root", 0);
  }

  static semantics::class_&
  polymorphic_base (semantics::class_& c)
  {
    // Set by the validator.
    //
    return *c.get<semantics::class_*> ("polymorphic-base");
  }

  static size_t
  polymorphic_depth (semantics::class_&);

  static bool
  discriminator (semantics::data_member& m)
  {
    return m.count ("discriminator");
  }

  static semantics::data_member*
  discriminator (semantics::class_& c)
  {
    // Set by type processor.
    //
    return c.get<semantics::data_member*> ("discriminator", 0);
  }

  // Model version.
  //
  bool
  versioned () const
  {
    return unit.count ("model-version") != 0;
  }

  model_version const&
  version () const
  {
    return unit.get<model_version> ("model-version");
  }

  // Versioned object, view, or composite.
  //
  static bool
  versioned (semantics::class_& c)
  {
    // Set by processor.
    //
    return c.count ("versioned") != 0;
  }

  // Versioned container.
  //
  static bool
  versioned (semantics::data_member& m)
  {
    // Set by processor.
    //
    return container (m)->count ("versioned");
  }

  // Object sections.
  //
  static object_section&
  section (semantics::data_member& m)
  {
    object_section* s (m.get<object_section*> ("section", 0));
    return s == 0 ? main_section : *s;
  }

  static object_section&
  section (data_member_path const& mp)
  {
    // The direct member of the object specifies the section. If the
    // path is empty (which can happen, for example, for a container
    // element), assume it is the main section.
    //
    //
    return mp.empty () ? main_section : section (*mp.front ());
  }

  // Member belongs to a section that is loaded separately.
  //
  static bool
  separate_load (semantics::data_member& m)
  {
    return section (m).separate_load ();
  }

  static bool
  separate_load (data_member_path const& mp)
  {
    return section (mp).separate_load ();
  }

  // Member belongs to a section that is updated separately.
  //
  static bool
  separate_update (semantics::data_member& m)
  {
    return section (m).separate_update ();
  }

  static bool
  separate_update (data_member_path const& mp)
  {
    return section (mp).separate_update ();
  }

  //
  //
  typedef ::class_kind class_kind_type;

  static class_kind_type
  class_kind (semantics::class_&);

  // Return class names. For ordinary classes, this will be the class
  // name itself. For class template instantiations this will be the
  // typedef name used in the pragma.
  //
  static string
  class_name (semantics::class_&);

  static string
  class_fq_name (semantics::class_&);

  // Return class scope. For ordinary classes, this will be the scope
  // where the class is defined. For class template instantiations this
  // will be the scope of the typedef name used in the pragma.
  //
  static semantics::scope&
  class_scope (semantics::class_&);

  // Return the class file. For ordinary classes, this will be the file
  // where the class is defined. For class template instantiations this
  // will be the file containing the pragma.
  //
  static semantics::path
  class_file (semantics::class_&);

  // Return the location (as location_t; useful for comparison) of
  // an "ODB class" (i.e., an object, view, or composite value),
  // taking into account things like definition point overrides,
  // etc.
  //
  location_t
  class_location (semantics::class_&);

  // Same as above, but returns "real" location, that is, ignoring
  // the definition point overrides.
  //
  location_t
  class_real_location (semantics::class_&);

  // Database names and types.
  //
public:
  // Schema name for a namespace.
  //
  qname
  schema (semantics::scope&) const;

  // Table name prefix for a namespace.
  //
  string
  table_name_prefix (semantics::scope&) const;

  //
  //
  struct table_prefix
  {
    table_prefix (): level (0), derived (false) {}
    table_prefix (semantics::class_&);

    void
    append (semantics::data_member&);

    qname ns_schema;  // Object's namespace schema.
    string ns_prefix; // Object's namespace table prefix.
    qname prefix;
    size_t level;
    bool derived;     // One of the components in the prefix was derived.
  };

  qname
  table_name (semantics::class_&, bool* derived = 0) const;

  qname
  table_name (semantics::class_&, data_member_path const&) const;

  // Table name for the container member. The table prefix passed as the
  // second argument must include the table prefix specified with the
  // --table-prefix option.
  //
  qname
  table_name (semantics::data_member&, table_prefix const&) const;

  string
  table_options (semantics::class_&);

  // Table options for the container member.
  //
  string
  table_options (semantics::data_member&, semantics::type& ct);

  //
  //
  struct column_prefix
  {
    column_prefix (): derived (false), underscore (false) {}

    column_prefix (semantics::data_member& m,
                   string const& key_prefix = string (),
                   string const& default_name = string ())
        : derived (false), underscore (false)
    {
      append (m, key_prefix, default_name);
    }

    // If the last argument is true, the prefix will include the last member
    // in the path.
    //
    column_prefix (data_member_path const&, bool last = false);

    bool
    empty () const {return prefix.empty ();}

    void
    append (semantics::data_member&,
            string const& key_prefix = string (),
            string const& default_name = string ());

    string prefix;
    bool derived;     // One of the components in the prefix was derived.
    bool underscore;  // Trailing underscore was automatically added.
  };

  string
  column_name (semantics::data_member&, bool& derived) const;

  string
  column_name (semantics::data_member&, column_prefix const&) const;

  string
  column_name (semantics::data_member&,
               string const& key_prefix,
               string const& default_name,
               bool& derived) const;

  string
  column_name (semantics::data_member&,
               string const& key_prefix,
               string const& default_name,
               column_prefix const&) const;

  string
  column_name (data_member_path const&) const;

  //
  //
  string
  column_type (const data_member_path&,
               string const& key_prefix = string (),
               bool id = false); // Pass true if this type is object id other
                                 // than because of the members in the path.
  string
  column_type (semantics::data_member&, string const& key_prefix = string ());

  string
  column_options (semantics::data_member&);

  string
  column_options (semantics::data_member&, string const& key_prefix);

  // Cleaned-up member name that can be used for database names.
  //
  string
  public_name_db (semantics::data_member&) const;

  // Compose the name by inserting/removing an underscore, as necessary.
  //
  static string
  compose_name (string const& prefix, string const& name);

  // SQL name transformations.
  //
  enum sql_name_type
  {
    sql_name_all,
    sql_name_table,
    sql_name_column,
    sql_name_index,
    sql_name_fkey,
    sql_name_sequence,
    sql_name_statement,
    sql_name_count
  };

  string
  transform_name (string const& name, sql_name_type) const;

  // C++ names.
  //
public:
  // Cleaned-up and potentially escaped member name that can be used
  // in public C++ interfaces.
  //
  string
  public_name (semantics::data_member&, bool escape = true) const;

  // "Flatten" fully-qualified C++ name by replacing '::' with '_'
  // and removing leading '::', if any.
  //
  static string
  flat_name (string const& fqname);

  // Escape C++ keywords, reserved names, and illegal characters.
  //
  string
  escape (string const&) const;

  // Make C++ include guard name by split words, e.g., "FooBar" to
  // "Foo_Bar" and converting everything to upper case.
  //
  string
  make_guard (string const&) const;

  // Return a string literal that can be used in C++ source code. It
  // includes "".
  //
  static string
  strlit (string const&);

public:
  // Generate explicit instantiation headers with all the necessary
  // extern and export symbols.
  //
  void
  inst_header (bool decl, bool omit_exp = false);

  // Counts and other information.
  //
public:
  struct column_count_type
  {
    column_count_type ()
        : total (0),
          id (0),
          inverse (0),
          readonly (0),
          optimistic_managed (0),
          discriminator (0),
          added (0),
          deleted (0),
          soft (0),
          separate_load (0),
          separate_update (0)
    {
    }

    size_t total;
    size_t id;
    size_t inverse;
    size_t readonly;
    size_t optimistic_managed;
    size_t discriminator;

    size_t added;   // Soft-added.
    size_t deleted; // Soft-deleted.
    size_t soft;    // Soft-added/deleted (a column can be both).

    size_t separate_load;
    size_t separate_update; // Only readwrite.
  };

  static column_count_type
  column_count (semantics::class_&, object_section* = 0);

  static data_member_path*
  id_member (semantics::class_& c)
  {
    // Set by the processor. May not be there for reuse-abstract
    // classes or classes without object id.
    //
    return c.count ("id-member") ? &c.get<data_member_path> ("id-member") : 0;
  }

  // Object pointer information.
  //
public:
  typedef ::pointer_kind pointer_kind_type;

  pointer_kind_type
  pointer_kind (semantics::type& p)
  {
    return p.get<pointer_kind_type> ("pointer-kind");
  }

  bool
  lazy_pointer (semantics::type& p)
  {
    return p.get<bool> ("pointer-lazy");
  }

  bool
  weak_pointer (semantics::type& p)
  {
    return pointer_kind (p) == pk_weak;
  }

  static data_member_path*
  inverse (semantics::data_member& m)
  {
    return object_pointer (utype (m)) && m.count ("inverse")
      ? &m.get<data_member_path> ("inverse")
      : 0;
  }

  data_member_path*
  inverse (semantics::data_member& m, string const& key_prefix)
  {
    if (key_prefix.empty ())
      return inverse (m);

    if (!object_pointer (utype (m, key_prefix)))
      return 0;

    string k (key_prefix + "-inverse");
    return m.count (k) ? &m.get<data_member_path> (k) : 0;
  }

  // Container information.
  //
public:
  typedef ::container_kind container_kind_type;

  static container_kind_type
  container_kind (semantics::type& c)
  {
    return c.get<container_kind_type> ("container-kind");
  }

  static bool
  container_smart (semantics::type& c)
  {
    return c.get<bool> ("container-smart");
  }

  static semantics::type&
  container_idt (semantics::data_member& m, const custom_cxx_type** trans = 0)
  {
    return utype (m, "id", trans);
  }

  static semantics::type&
  container_vt (semantics::data_member& m, const custom_cxx_type** trans = 0)
  {
    return utype (m, "value", trans);
  }

  static semantics::type&
  container_it (semantics::data_member& m, const custom_cxx_type** trans = 0)
  {
    return utype (m, "index", trans);
  }

  static semantics::type&
  container_kt (semantics::data_member& m, const custom_cxx_type** trans = 0)
  {
    return utype (m, "key", trans);
  }

  static bool
  unordered (semantics::data_member& m)
  {
    if (m.count ("unordered"))
      return true;

    if (semantics::type* c = container (m))
      return c->count ("unordered");

    return false;
  }

  // The 'is a' and 'has a' tests. The has_a() test currently does not
  // cross the container boundaries.
  //
public:
  static unsigned short const test_pointer = 0x01;
  static unsigned short const test_eager_pointer = 0x02;
  static unsigned short const test_lazy_pointer = 0x04;
  static unsigned short const test_container = 0x08;
  static unsigned short const test_straight_container = 0x10;
  static unsigned short const test_inverse_container = 0x20;
  static unsigned short const test_readonly_container = 0x40;
  static unsigned short const test_readwrite_container = 0x80;
  static unsigned short const test_smart_container = 0x100;

  // Exclude versioned containers.
  //
  static unsigned short const exclude_versioned = 0x200;

  // Treat eager loaded members as belonging to the main section.
  // If this flag is specified, then section must be main_section.
  //
  static unsigned short const include_eager_load = 0x800;

  // Exclude added/deleted members.
  //
  static unsigned short const exclude_added   = 0x1000;
  static unsigned short const exclude_deleted = 0x2000;

  // By default the test goes into bases for non-polymorphic
  // hierarchies and doesn't go for polymorphic. The following
  // flags can be used to alter this behavior.
  //
  static unsigned short const exclude_base = 0x4000;
  static unsigned short const include_base = 0x8000;

  bool
  is_a (data_member_path const& mp,
        data_member_scope const& ms,
        unsigned short flags)
  {
    return is_a (mp, ms, flags, utype (*mp.back ()), "");
  }

  bool
  is_a (data_member_path const&,
        data_member_scope const&,
        unsigned short flags,
        semantics::type&,
        string const& key_prefix);

  // Return the number of matching entities. Can be used as a just
  // a bool value (0 means no match).
  //
  size_t
  has_a (semantics::class_&, unsigned short flags, object_section* = 0);

public:
  // Process include path by adding the prefix, putting it through
  // the include regex list, and adding opening and closing include
  // characters ("" or <>) if necessary. The prefix argument indicates
  // whether the include prefix specified with the --include-prefix
  // option should be added. The open argument can be used to specify
  // the opening character. It can have three values: ", <, or \0. In
  // case of \0, the character is determined based on the value of the
  // --include-with-bracket option.
  //
  string
  process_include_path (string const&, bool prefix = true, char open = '\0');

  // Diverge output.
  //
public:
  void
  diverge (std::ostream& os)
  {
    diverge (os.rdbuf ());
  }

  void
  diverge (std::streambuf* sb);

  void
  restore ();

  // Implementation details.
  //
private:
  static bool
  composite_ (semantics::class_&);

  template <typename X>
  static X
  indirect_value (semantics::context const& c, string const& key)
  {
    typedef X (*func) ();
    std::type_info const& ti (c.type_info (key));

    if (ti == typeid (func))
      return c.get<func> (key) ();
    else
      return c.get<X> (key);
  }

  static semantics::type*
  indirect_type (semantics::context const& c,
                 string const& kp,
                 semantics::names*& hint)
  {
    typedef semantics::type* (*func) (semantics::names*&);

    string const tk (kp + "-tree-type");
    std::type_info const& ti (c.type_info (tk));

    if (ti == typeid (func))
      return c.get<func> (tk) (hint);
    else
    {
      hint = c.get<semantics::names*> (kp + "-tree-hint");
      return c.get<semantics::type*> (tk);
    }
  }

public:
  typedef std::set<string> keyword_set_type;

  struct db_type_type
  {
    db_type_type () {}
    db_type_type (string const& t, string const& it, bool n)
        : type (t), id_type (it), null (n)
    {
    }

    string type;
    string id_type;
    bool null;
  };

  struct type_map_type: std::map<string, db_type_type>
  {
    typedef std::map<string, db_type_type> base;

    const_iterator
    find (semantics::type&, semantics::names* hint);
  };

protected:
  struct data
  {
    virtual
    ~data () {}

    data (std::ostream& os)
        : extra_ (0),
          os_ (os.rdbuf ()),
          in_comment_ (false),
          top_object_ (0),
          cur_object_ (0),
          sql_name_upper_ ("(.+)", "\\U$1"),
          sql_name_lower_ ("(.+)", "\\L$1")
    {
    }

  public:
    void* extra_;

    std::ostream os_;
    std::stack<std::streambuf*> os_stack_;

    bool in_comment_;

    semantics::class_* top_object_;
    semantics::class_* cur_object_;

    string exp_;
    string ext_;

    keyword_set_type keyword_set_;
    type_map_type type_map_;

    regex_mapping sql_name_regex_[sql_name_count];
    regexsub sql_name_upper_;
    regexsub sql_name_lower_;

    regex_mapping include_regex_;
    regex_mapping accessor_regex_;
    regex_mapping modifier_regex_;
  };

  typedef cutl::shared_ptr<data> data_ptr;
  data_ptr data_;

public:
  typedef ::features features_type;

  void*& extra; // Extra data that may need to be shared by a sub-system.

  std::ostream& os;
  semantics::unit& unit;
  options_type const& options;
  features_type& features;
  database const db;

  bool& in_comment;

  string& exp; // Export symbol (with trailing space if specified).
  string& ext; // Extern symbol.

  keyword_set_type const& keyword_set;

  regex_mapping const& include_regex;
  regex_mapping const& accessor_regex;
  regex_mapping const& modifier_regex;

  bool embedded_schema;
  bool separate_schema;

  bool multi_static;
  bool multi_dynamic;

  bool force_versioned; // Force statement processing for debugging.

  // Outermost object or view currently being traversed.
  //
  semantics::class_*& top_object;

  // Object or view currently being traversed. It can be the same as
  // top_object or it can a base of top_object.
  //
  semantics::class_*& cur_object;

  // Per-database customizable functionality.
  //
protected:
  // Return empty string if there is no mapping. The type passed is
  // already cvr-unqualified. The null out argument indicates whether
  // the column should allow NULL values by default.
  //
  string
  database_type (semantics::type& t,
                 semantics::names* hint,
                 bool id,
                 bool* null = 0)
  {
    return current ().database_type_impl (t, hint, id, null);
  }

  // The default implementation uses the type map (populated by the database-
  // specific context implementation) to come up with a mapping.
  //
  virtual string
  database_type_impl (semantics::type&, semantics::names*, bool, bool*);

public:
  typedef context root_context;

  virtual
  ~context ();
  context ();
  context (std::ostream&,
           semantics::unit&,
           options_type const&,
           features_type&,
           data_ptr = data_ptr ());

  static context&
  current ()
  {
    return *current_;
  }

private:
  static context* current_;

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
  context&
  operator= (context const&) = delete;
#else
private:
  context&
  operator= (context const&);
#endif
};

// Create concrete database context.
//
std::unique_ptr<context>
create_context (std::ostream&,
                semantics::unit&,
                options const&,
                features&,
                semantics::relational::model*);

// Checks if scope Y names any of X.
//
template <typename X, typename Y>
bool
has (Y& y)
{
  for (semantics::scope::names_iterator i (y.names_begin ()),
         e (y.names_end ()); i != e; ++i)
    if (i->named (). template is_a<X> ())
      return true;

  return false;
}

#include <odb/context.ixx>

#endif // ODB_CONTEXT_HXX
