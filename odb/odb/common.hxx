// file      : odb/common.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_COMMON_HXX
#define ODB_COMMON_HXX

#include <string>
#include <vector>
#include <cstddef> // std::size_t
#include <cassert>

#include <odb/context.hxx>
#include <odb/instance.hxx>

// Traverse object members recursively by going into bases and
// composite members.
//
struct object_members_base: traversal::class_, virtual context
{
  virtual void
  traverse_simple (semantics::data_member&);

  // Traverse object pointer. The second argument is the pointed-to
  // class. When overriding this function, you will most likely want
  // to call the base in order to traverse the pointer as a simple
  // member (simple object id) or as composite (composite object id).
  //
  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

  // If you override this function, you can call the base to traverse
  // bases and members. The first argument is the data member and can
  // be NULL if we are traversing the root type or a base. The second
  // argument is the actual composite type, which is not necessarily
  // the same as the member's type.
  //
  virtual void
  traverse_composite (semantics::data_member*, semantics::class_&);

  // More general version of the above function that allows detection
  // of wrapped composite value. By default this function calls
  // traverse_composite (m, comp) ignoring the wrapper type. Note that
  // this function is called for all composite value (wrapped or not).
  // If it is not wrapped, the wrapper argument will be NULL.
  //
  virtual void
  traverse_composite_wrapper (semantics::data_member*,
                              semantics::class_& comp,
                              semantics::type* wrapper);

  // The second argument is the actual container type in case the
  // member type is a wrapper.
  //
  virtual void
  traverse_container (semantics::data_member&, semantics::type&);

  // If you override this function, you can call the base to traverse
  // bases and members.
  //
  virtual void
  traverse_object (semantics::class_&);

  // If you override this function, you can call the base to traverse
  // members.
  //
  virtual void
  traverse_view (semantics::class_&);

  virtual bool
  section_test (data_member_path const&);

public:
  object_members_base (bool traverse_poly_base = false,
                       object_section* section = 0)
      : section_ (section), top_level_ (true), member_ (*this)
  {
    init (false, false, false, traverse_poly_base);
  }

  object_members_base (bool build_flat_prefix,
                       bool build_table_prefix,
                       bool build_member_prefix,
                       bool traverse_poly_base = false,
                       object_section* section = 0)
      : section_ (section), top_level_ (true), member_ (*this)
  {
    init (build_flat_prefix,
          build_table_prefix,
          build_member_prefix,
          traverse_poly_base);
  }

  object_members_base (object_members_base const& x)
      : context (), //@@ -Wextra
        section_ (x.section_),
        top_level_ (true),
        member_ (*this)
  {
    init (x.build_flat_prefix_,
          x.build_table_prefix_,
          x.build_member_prefix_,
          x.traverse_poly_base_);
  }

  virtual void
  traverse (semantics::class_&);

protected:
  string flat_prefix_;
  table_prefix table_prefix_;
  string member_prefix_;

  data_member_path member_path_;
  data_member_scope member_scope_;

  object_section* section_;

protected:
  semantics::data_member*
  id () const
  {
    assert (!member_path_.empty ());
    return context::id (member_path_);
  }

private:
  void
  init (bool build_flat_prefix,
        bool build_table_prefix,
        bool build_member_prefix,
        bool traverse_poly_base)
  {
    build_flat_prefix_ = build_flat_prefix;
    build_table_prefix_ = build_table_prefix;
    build_member_prefix_ = build_member_prefix;
    traverse_poly_base_ = traverse_poly_base;

    *this >> names_ >> member_;
    *this >> inherits_ >> *this;
  }

private:
  virtual void
  traverse_member (semantics::data_member&, semantics::type&);

  struct member: traversal::data_member
  {
    member (object_members_base& om): om_ (om) {}

    virtual void
    traverse (semantics::data_member&);

  public:
    object_members_base& om_;
  };

  bool build_flat_prefix_;
  bool build_table_prefix_;
  bool build_member_prefix_;

  bool traverse_poly_base_;

  bool top_level_;

  member member_;
  traversal::names names_;
  traversal::inherits inherits_;
};

// Traverse object columns recursively by going into composite members
// and bases.
//
struct object_columns_base: traversal::class_, virtual context
{
  // Returning false means that the column has been ignored and the
  // first flag should not be changed.
  //
  virtual bool
  traverse_column (semantics::data_member&,
                   string const& name,
                   bool first);

  // Traverse object pointer. The second argument is the pointed-to
  // class. When overriding this function, you will most likely want
  // to call the base in order to traverse the member as a column
  // (simple object id) or columns (composite object id).
  //
  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

  virtual void
  traverse_points_to (semantics::data_member&, semantics::class_&);

  // If you override this function, you can call the base to traverse
  // bases and members. The first argument is the data member and can
  // be NULL if we are traversing the root type or a base. The second
  // argument is the actual composite type, which is not necessarily
  // the same as the member type.
  //
  virtual void
  traverse_composite (semantics::data_member*, semantics::class_&);

  // If you override this function, you can call the base to traverse
  // bases and members.
  //
  virtual void
  traverse_object (semantics::class_&);

  // If you override this function, you can call the base to traverse
  // members.
  //
  virtual void
  traverse_view (semantics::class_&);

  // Called after the last column, provided at least one column hasn't
  // been ignored.
  //
  virtual void
  flush ();

  virtual bool
  section_test (data_member_path const&);

  // Start/end traversal callbacks.
  //
  virtual void
  traverse_pre (semantics::nameable&);

  virtual void
  traverse_post (semantics::nameable&);

public:
  object_columns_base (bool first = true,
                       column_prefix const& cp = column_prefix (),
                       object_section* section = 0)
      : column_prefix_ (cp),
        section_ (section),
        root_ (0),
        traverse_poly_base_ (false),
        first_ (first),
        top_level_ (true),
        member_ (*this)
  {
    init ();
  }

  object_columns_base (bool first,
                       bool traverse_poly_base,
                       object_section* section = 0)
      : section_ (section),
        root_ (0),
        traverse_poly_base_ (traverse_poly_base),
        first_ (first),
        top_level_ (true),
        member_ (*this)
  {
    init ();
  }

  object_columns_base (object_columns_base const& x)
      : context (), //@@ -Wextra
        column_prefix_ (x.column_prefix_),
        section_ (x.section_),
        root_ (0),
        traverse_poly_base_ (x.traverse_poly_base_),
        first_ (x.first_),
        top_level_ (true),
        member_ (*this)
  {
    init ();
  }

  virtual void
  traverse (semantics::class_&);

  // Traverse a data member with type, which can be a simple or composite
  // value type, or an object pointer (with a simple or composite id).
  //
  virtual void
  traverse (semantics::data_member&);

  virtual void
  traverse (semantics::data_member& m, column_prefix const& cp)
  {
    column_prefix op (column_prefix_);
    column_prefix_ = cp;
    traverse (m);
    column_prefix_ = op;
  }

  virtual void
  traverse (data_member_path& mp)
  {
    data_member_path op (member_path_);
    member_path_ = mp;
    traverse (*mp.back (), column_prefix (mp));
    member_path_ = op;
  }

  // Should only be used for containers.
  //
  virtual void
  traverse (semantics::data_member&,
            semantics::type&,
            string const& key_prefix,
            string const& default_name,
            semantics::class_* top_object = 0); // If not 0, switch top object.

protected:
  string key_prefix_;
  string default_name_;

  column_prefix column_prefix_;

  data_member_path member_path_;
  data_member_scope member_scope_;

  object_section* section_;

protected:
  semantics::data_member*
  id () const
  {
    if (root_ != 0)
      return root_id_ ? root_ : 0; // Cannot have ids below root.
    else
    {
      assert (!member_path_.empty ());
      return context::id (member_path_);
    }
  }

  string
  column_type ()
  {
    if (member_path_.empty ())
    {
      assert (root_ != 0);
      return context::column_type (*root_, key_prefix_);
    }
    else
      return context::column_type (
        member_path_, key_prefix_, root_ != 0 && (root_id_ || root_op_));
  }

  bool
  null () const
  {
    return (root_ != 0 && root_null_) || context::null (member_path_);
  }

private:
  semantics::data_member* root_; // Root member if traversing from a member.
  bool root_id_;                 // True if traversing root as object id.
  bool root_op_;                 // True if traversing root as object pointer.
  bool root_null_;               // True if root is null-able.

private:
  void
  init ()
  {
    *this >> names_ >> member_;
    *this >> inherits_ >> *this;
  }

private:
  virtual void
  traverse_member (semantics::data_member&, semantics::type&);

  struct member: traversal::data_member, context
  {
    member (object_columns_base& oc): oc_ (oc) {}

    virtual void
    traverse (semantics::data_member&);

  public:
    object_columns_base& oc_;
  };

  bool traverse_poly_base_;

  bool first_;
  bool top_level_;

  member member_;
  traversal::names names_;
  traversal::inherits inherits_;
};

struct object_columns_list: object_columns_base
{
  typedef object_columns_list base;

  object_columns_list (bool ignore_inverse = true)
      : ignore_inverse_ (ignore_inverse)
  {
  }

  object_columns_list (column_prefix const& cp, bool ignore_inverse = true)
      : object_columns_base (true, cp), ignore_inverse_ (ignore_inverse)
  {
  }

  struct column
  {
    column (std::string const& n,
            std::string const& t,
            semantics::data_member& m)
        : name (n), type (t), member (&m)
    {
    }

    std::string name;
    std::string type;
    semantics::data_member* member;
  };

  typedef std::vector<column> columns;
  typedef columns::const_iterator iterator;

  iterator
  begin () const {return columns_.begin ();}

  iterator
  end () const {return columns_.end ();}

  columns::size_type
  size () const {return columns_.size ();}

  virtual void
  traverse_pointer (semantics::data_member&, semantics::class_&);

  virtual bool
  traverse_column (semantics::data_member&, string const&, bool);

private:
  bool ignore_inverse_;
  columns columns_;
};

// Traverse objects, views, and composite values that are class template
// instantiations.
//
struct typedefs: traversal::typedefs, context
{
  typedefs (bool traverse_included)
      : included_ (traverse_included)
  {
  }

  virtual void
  traverse (semantics::typedefs&);

  // Returns true if we should traverse this typedef.
  //
  bool
  check (semantics::typedefs&);

private:
  bool included_;
};

// Other common parts.
//
#include <odb/common-query.hxx>

#endif // ODB_COMMON_HXX
