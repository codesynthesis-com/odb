// file      : odb/pragma.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_PRAGMA_HXX
#define ODB_PRAGMA_HXX

#include <odb/gcc.hxx>

#include <map>
#include <set>
#include <vector>
#include <string>

#include <odb/option-types.hxx> // database

#include <libcutl/container/any.hxx>
#include <libcutl/container/multi-index.hxx>
#include <libcutl/compiler/context.hxx>

struct virt_declaration
{
  virt_declaration (location_t l,
                    location_t o,
                    int ob,
                    std::string const& n,
                    gcc_tree_code_type tc,
                    tree t)
      : loc (l), ord (o), ord_bias (ob), name (n), tree_code (tc), type (t) {}

  location_t loc;
  location_t ord; // Ordering location for before/after support.
  int ord_bias;   // Ordering bias for the same locations.
  std::string name;
  gcc_tree_code_type tree_code;
  tree type;      // Declaration's type.
};

// Note that we consider virtual declarations with the same name but
// different tree codes unequal. If that is too loose, then the
// inserting code must do additional checks.
//
struct virt_declaration_set
{
  typedef cutl::container::key<std::string, gcc_tree_code_type> key;
  typedef std::map<key, virt_declaration> map;
  typedef cutl::container::map_const_iterator<map> const_iterator;

  std::pair<const_iterator, bool>
  insert (const virt_declaration& v)
  {
    std::pair<map::iterator, bool> r (
      map_.insert (map::value_type (key (v.name, v.tree_code), v)));

    const_iterator i (r.first);

    if (r.second)
      r.first->first.assign (i->name, i->tree_code);

    return std::make_pair (i, r.second);
  }

  const_iterator
  find (std::string const& name, gcc_tree_code_type tree_code) const
  {
    return map_.find (key (name, tree_code));
  }

  const_iterator begin () const {return map_.begin ();}
  const_iterator end () const {return map_.end ();}

private:
  map map_;
};

// Map of scopes (e.g., class, namespace) to sets of virtual declarations.
//
typedef std::map<tree, virt_declaration_set> virt_declarations;
extern virt_declarations virt_declarations_;

// Real or virtual declaration. If it is real, then it is a pointer to
// the GCC tree node. Otherwise, it is a pointer to virt_declaration
// from virt_declarations_ above.
//
struct declaration
{
  declaration (): virt (false) {decl.real = 0;}
  declaration (tree d): virt (false) {decl.real = d;}
  declaration (virt_declaration const& d): virt (true) {decl.virt = &d;}

  bool virt;

  union
  {
    tree real;
    virt_declaration const* virt;
  } decl;

  gcc_tree_code_type
  tree_code () const
  {
    return (virt ? decl.virt->tree_code : TREE_CODE (decl.real));
  }

  typedef bool declaration::*bool_convertible;
  operator bool_convertible () const
  {
    return ptr () == 0 ? 0 : &declaration::virt;
  }

public:
  bool
  operator== (declaration const& x) const
  {
    return virt == x.virt && ptr () == x.ptr ();
  }

  bool
  operator!= (declaration const& x) const
  {
    return !(*this == x);
  }

  bool
  operator< (declaration const& x) const
  {
    return virt < x.virt || (virt == x.virt && ptr () < x.ptr ());
  }

public:
  void const*
  ptr () const
  {
    return virt
      ? static_cast<void const*> (decl.virt)
      : static_cast<void const*> (decl.real);
  }
};

inline bool
operator== (declaration const& x, tree y)
{
  return !x.virt && x.decl.real == y;
}

inline bool
operator== (tree x, declaration const& y)
{
  return !y.virt && y.decl.real == x;
}

struct pragma
{
  // Check that the pragma is applicable to the declaration. Return true
  // on success, complain and return false otherwise.
  //
  typedef bool (*check_func) (declaration const& decl,
                              std::string const& decl_name,
                              std::string const& prag_name,
                              location_t);

  // Add the pragma value to the context.
  //
  typedef void (*add_func) (cutl::compiler::context&,
                            std::string const& key,
                            cutl::container::any const& value,
                            location_t);

  pragma (std::string const& pn,
          std::string const& cn,
          cutl::container::any const& v,
          location_t l,
          check_func c,
          add_func a)
    : pragma_name (pn),
      context_name (cn),
      value (v),
      loc (l),
      check (c),
      add (a)
  {
  }

  std::string pragma_name;  // Actual pragma name for diagnostics.
  std::string context_name; // Context entry name.
  cutl::container::any value;
  location_t loc;
  check_func check;
  add_func add;
};

typedef std::vector<pragma> pragma_list;

// A set of pragmas. Insertion of a pragma with the same name and no
// custom add function overrides the old value.
//
struct pragma_set: std::multimap<std::string, pragma>
{
  typedef std::multimap<std::string, pragma> base;

  pragma&
  insert (pragma const& p)
  {
    std::string const& n (p.context_name);
    std::pair<iterator, iterator> r (equal_range (n));

    iterator i (end ());

    if (p.add == 0)
    {
      if (r.first != r.second)
      {
        i = r.first;
        assert (++r.first == r.second);

        i->second = p;
      }
    }
    else if (r.first != r.second)
      assert ((--r.second)->second.loc <= p.loc);

    if (i == end ())
      i = base::insert (base::value_type (n, p));

    return i->second;
  }

  void
  insert (const_iterator begin, const_iterator end)
  {
    for (; begin != end; ++begin)
      insert (begin->second);
  }

  // Return the last pragma in the equal range which (by construction) has the
  // location greater or equal to all the other pragmas in this range.
  //
  iterator
  find (std::string const& n)
  {
    return equal_range (n).second;
  }
};


// Position pragmas inside a class or namespace. The key for the
// namespace case is the global_namespace node.
//
typedef std::map<tree, pragma_list> loc_pragmas;
extern loc_pragmas loc_pragmas_;

// Position pragmas for namespaces. Because re-opened namespaces do
// not have any representation in the GCC tree, these are handled in
// a special way. They are stored as a list of pragmas and their outer
// namespaces.
//
struct ns_loc_pragma
{
  typedef ::pragma pragma_type;
  ns_loc_pragma (tree n, pragma_type const& p): ns (n), pragma (p) {}

  tree ns;
  pragma_type pragma;
};

typedef std::vector<ns_loc_pragma> ns_loc_pragmas;
extern ns_loc_pragmas ns_loc_pragmas_;

// Pragmas associated with specific declarations (real or virtual).
//
typedef std::map<declaration, pragma_set> decl_pragmas;
extern decl_pragmas decl_pragmas_;

// Database we are generating code for as well as multi-database support.
// Used to ignore database-specific pragmas.
//
extern database pragma_db_;
extern multi_database pragma_multi_;

extern "C" void
register_odb_pragmas (void*, void*);

struct pragmas_failed {};

void
post_process_pragmas ();

#endif // ODB_PRAGMA_HXX
