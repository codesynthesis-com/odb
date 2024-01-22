// file      : odb/common-query.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_COMMON_QUERY_HXX
#define ODB_COMMON_QUERY_HXX

#include <odb/common.hxx>
#include <odb/context.hxx>

//
// Query-related generators.
//

struct query_utils: virtual context
{
  void
  inst_query_columns (bool decl, // Extern declaration or instanatiation.
                      bool ptr,  // pointer_query_columns or query_columns
                      string const& type,    // Object fq-type.
                      string const& alias,   // Table alias.
                      semantics::class_&);   // Traverse for nested structs.

  static string
  depth_suffix (size_t);
};

// Generate query tags for pointers in this object.
//
struct query_tags: object_columns_base, virtual context
{
  typedef query_tags base;

  query_tags (): nl_ (false), depth_ (0) {}

  virtual void
  traverse (semantics::class_&);

  virtual void
  traverse_object (semantics::class_&);

  virtual void
  traverse_composite (semantics::data_member*, semantics::class_&);

  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

  virtual void
  generate (string const& name);

private:
  bool nl_;
  size_t depth_;
};

// Generate alias_traits specializations for pointers in this objects.
//
struct query_alias_traits: object_columns_base, virtual context
{
  typedef query_alias_traits base;

  query_alias_traits (semantics::class_&, bool decl);

  virtual void
  traverse_object (semantics::class_&);

  virtual void
  traverse_composite (semantics::data_member*, semantics::class_&);

  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

  virtual void
  generate_decl (string const& tag, semantics::class_&);

  virtual void
  generate_decl_body ();

  virtual void
  generate_def (semantics::data_member&, semantics::class_&);

  virtual void
  generate_def (string const& tag, semantics::class_&, string const& alias);

protected:
  bool decl_;
  string scope_;
  size_t depth_;
};

// Generate query columns in the query_columns_base class.
//
struct query_columns_base: object_columns_base, query_utils
{
  typedef query_columns_base base;

  // If inst is true, then we generate extern template declarations
  // in the header.
  //
  query_columns_base (semantics::class_&, bool decl, bool inst);

  virtual void
  traverse_object (semantics::class_&);

  virtual void
  traverse_composite (semantics::data_member*, semantics::class_&);

  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

  virtual void
  generate_inst (semantics::data_member&, semantics::class_&);

protected:
  bool decl_;
  bool inst_;
  string const_; // Const prefix or empty.
  string scope_;
  size_t depth_;
};

// Generate query columns in the query_columns or pointer_query_columns
// class.
//
struct query_columns: object_columns_base, virtual context
{
  typedef query_columns base;

  query_columns (bool decl, bool ptr, semantics::class_&);

  virtual void
  column_ctor (string const& type, string const& name, string const& base);

  virtual void
  traverse_object (semantics::class_&);

  virtual void
  traverse_composite (semantics::data_member*, semantics::class_&);

  virtual void
  column_common (semantics::data_member&,
                 string const& type,
                 string const& column,
                 string const& suffix = "_type_");

  virtual bool
  traverse_column (semantics::data_member&, string const&, bool);

  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

protected:
  bool decl_;
  bool ptr_;
  bool poly_ref_;
  string const_;   // Const prefix or empty.
  bool in_ptr_;    // True while we are "inside" an object pointer.
  string fq_name_;
  bool resue_abstract_;  // Object is reuse-abstract.
  string scope_;
  size_t depth_;
};

// Generate the list of base classes for the query_columns or
// pointer_query_columns class.
//
struct query_columns_bases: traversal::class_, virtual context
{
  typedef query_columns_bases base;

  query_columns_bases (bool ptr, bool first = true)
      : ptr_ (ptr), first_ (first) {}

  virtual void
  traverse (type&);

private:
  bool ptr_;
  bool first_;
};

// Generate the base class aliases (typedefs). These can be used to
// resolve member ambiguities.
//
struct query_columns_base_aliases: traversal::class_, virtual context
{
  typedef query_columns_base_aliases base;

  query_columns_base_aliases (bool ptr): ptr_ (ptr) {}

  virtual void
  traverse (type&);

private:
  bool ptr_;
};

// Generate explicit instantiations of base classes.
//
struct query_columns_base_insts: traversal::class_, query_utils
{
  typedef query_columns_base_insts base;

  query_columns_base_insts (bool test_ptr,
                            bool decl,
                            string const& alias,
                            bool poly); // Traverse polymorphic bases.
  query_columns_base_insts (query_columns_base_insts const&);

  virtual void
  traverse (type&);

private:
  bool test_ptr_;
  bool decl_;
  string alias_;
  bool poly_;
  traversal::inherits inherits_;
};

// Generate the query_columns_base/query_columns or pointer_query_columns
// classes for objects.
//
struct query_columns_type: traversal::class_, query_utils
{
  typedef query_columns_type base;

  // Depending on the ptr argument, generate query_columns or
  // pointer_query_columns specialization. The latter is used
  // for object pointers where we don't support nested pointers.
  //
  // If decl is false then generate definitions (only needed for
  // query_columns so ptr should be false).
  //
  // If inst if true, then generate extern template declarations
  // in the header (ptr and decl should be false).
  //
  query_columns_type (bool ptr, bool decl, bool inst)
      : ptr_ (ptr), decl_ (decl), inst_ (inst) {}

  virtual void
  traverse (type&);

  virtual void
  generate_impl (type&);

  virtual void
  generate_inst (type&);

public:
  bool ptr_;
  bool decl_;
  bool inst_;
};

// Generate the query_columns class for views.
//
struct view_query_columns_type: traversal::class_, query_utils
{
  typedef view_query_columns_type base;

  view_query_columns_type (bool decl): decl_ (decl) {}

  virtual void
  traverse (type&);

  void
  generate_decl (type&);

  void
  generate_def (type&);

  void
  generate_inst (type&);

public:
  bool decl_;
};

#endif // ODB_COMMON_QUERY_HXX
