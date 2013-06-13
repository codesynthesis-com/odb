// file      : odb/context.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_CONTEXT_HXX
#define ODB_CONTEXT_HXX

#include <odb/gcc-fwd.hxx>

#include <map>
#include <set>
#include <stack>
#include <vector>
#include <string>
#include <memory>  // std::auto_ptr
#include <ostream>
#include <cstddef> // std::size_t
#include <iostream>

#include <cutl/re.hxx>
#include <cutl/shared-ptr.hxx>

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

//
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
typedef std::vector<semantics::data_member*> data_member_path;

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

// Database potentially-qualified name.
//
using semantics::relational::qname;

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

  kind_type kind;
  tree obj_node;         // Tree node if kind is object.
  std::string obj_name;  // Name as specified in the pragma if kind is object.
  qname tbl_name;        // Table name if kind is table.
  std::string alias;
  tree scope;
  location_t loc;
  semantics::class_* obj;

  cxx_tokens cond; // Join condition tokens.
};

typedef std::vector<view_object> view_objects;

// The view_alias_map does not contain entries for tables.
//
typedef std::map<std::string, view_object*> view_alias_map;
typedef std::map<semantics::class_*, view_object*> view_object_map;

//
//
struct view_query
{
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
};

//
//
struct table_column
{
  qname table;
  std::string column;
  bool expr; // True if column is an expression, and therefore should not
             // be quoted.
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
  member_access (const location& l, bool s)
      : loc (l), synthesized (s), by_value (false) {}

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

  std::string
  translate (std::string const& obj,
             std::string const& val = std::string ()) const;

  location loc;
  bool synthesized; // If true, then this is a synthesized expression.
  cxx_tokens expr;
  bool by_value;   // True if accessor returns by value. False doesn't
                   // necessarily mean that it is by reference.
};

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

  // The same for a member's type.
  //
  static semantics::type&
  utype (semantics::data_member& m)
  {
    return utype (m.type ());
  }

  // In addition to the unqualified type, this version also returns the
  // name hint for this type. If the member type is already unqualified,
  // then the hint is from the belongs edge. Otherwise, it is from the
  // qualifies edge.
  //
  static semantics::type&
  utype (semantics::data_member&, semantics::names*& hint);

  // For arrays this function returns true if the (innermost) element
  // type is const.
  //
  static bool
  const_type (semantics::type&);

  static semantics::type&
  member_type (semantics::data_member&, string const& key_prefix);

  static semantics::type&
  member_utype (semantics::data_member& m, string const& key_prefix)
  {
    return utype (member_type (m, key_prefix));
  }

  // Form a reference type for a member type. If make_const is true, then
  // add top-level const qualifier, unless it is already there. If it is
  // false, then strip it if it is already there. If var is not empty,
  // then embed the variable name into the type (e.g., char (*v)[3]).
  //
  static string
  member_ref_type (semantics::data_member& m,
                   bool make_const,
                   string const& var = "")
  {
    return type_ref_type (m.type (), m.belongs ().hint (), make_const, var);
  }

  static string
  type_ref_type (semantics::type&,
                 semantics::names* hint,
                 bool make_const,
                 string const& var = "");

  // Form a value type for a member type. If make_const is true, then add
  // top-level const qualifier, unless it is already there. If it is false,
  // then strip it if it is already there. If var is not empty, then embed
  // the variable name into the type (e.g., char v[3]).
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
    return m.count ("auto");
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

  // Return the class file. For ordinary classes, this will be the file
  // where the class is defined. For class template instantiations this
  // will be the file containing the pragma.
  //
  static semantics::path
  class_file (semantics::class_&);

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

  //
  //
  struct column_prefix
  {
    column_prefix (): derived (false) {}

    column_prefix (semantics::data_member& m,
                   string const& key_prefix = string (),
                   string const& default_name = string ())
        : derived (false)
    {
      append (m, key_prefix, default_name);
    }

    // If the last argument is true, the prefix will include the last member
    // in the path.
    //
    column_prefix (data_member_path const&, bool last = false);

    void
    append (semantics::data_member&,
            string const& key_prefix = string (),
            string const& default_name = string ());

    string prefix;
    bool derived;     // One of the components in the prefix was derived.
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
  inst_header (bool decl);

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
          discriminator (0)
    {
    }

    size_t total;
    size_t id;
    size_t inverse;
    size_t readonly;
    size_t optimistic_managed;
    size_t discriminator;
  };

  static column_count_type
  column_count (semantics::class_&);

  static semantics::data_member*
  id_member (semantics::class_& c)
  {
    // Set by the validator. May not be there for reuse-abstract
    // classes or classes without object id.
    //
    return c.get<semantics::data_member*> ("id-member", 0);
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

  static semantics::data_member*
  inverse (semantics::data_member& m)
  {
    return object_pointer (utype (m))
      ? m.get<semantics::data_member*> ("inverse", 0)
      : 0;
  }

  semantics::data_member*
  inverse (semantics::data_member& m, string const& key_prefix)
  {
    if (key_prefix.empty ())
      return inverse (m);

    return object_pointer (member_utype (m, key_prefix))
      ? m.get<semantics::data_member*> (key_prefix + "-inverse", 0)
      : 0;
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
  container_idt (semantics::data_member& m)
  {
    return member_utype (m, "id");
  }

  static semantics::type&
  container_vt (semantics::type& c)
  {
    return *c.get<semantics::type*> ("value-tree-type");
  }

  static semantics::type&
  container_it (semantics::type& c)
  {
    return *c.get<semantics::type*> ("index-tree-type");
  }

  static semantics::type&
  container_kt (semantics::type& c)
  {
    return *c.get<semantics::type*> ("key-tree-type");
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

  // The 'is a' and 'has a' tests. The has_a test currently does not
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
  static unsigned short const test_smart_container = 0x80;

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

  // Return the number of matching entities. Can be uses as a just
  // a bool value (0 means no match).
  //
  size_t
  has_a (semantics::class_&, unsigned short flags);

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
        : os_ (os.rdbuf ()), top_object_ (0), cur_object_ (0),
          sql_name_upper_ ("(.+)", "\\U$1"),
          sql_name_lower_ ("(.+)", "\\L$1")
    {
    }

  public:
    std::ostream os_;
    std::stack<std::streambuf*> os_stack_;

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

  std::ostream& os;
  semantics::unit& unit;
  options_type const& options;
  features_type& features;
  database const db;

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

private:
  context&
  operator= (context const&);
};

// Create concrete database context.
//
std::auto_ptr<context>
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

#endif // ODB_CONTEXT_HXX
