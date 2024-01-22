// file      : odb/semantics/class.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_CLASS_HXX
#define ODB_SEMANTICS_CLASS_HXX

#include <vector>
#include <odb/semantics/elements.hxx>

namespace semantics
{
  class class_;

  class inherits: public edge
  {
  public:
    typedef semantics::access access_type;

    class_&
    base () const
    {
      return *base_;
    }

    class_&
    derived () const
    {
      return *derived_;
    }

    bool
    virtual_ () const
    {
      return virt_;
    }

    access_type
    access () const
    {
      return access_;
    }

  public:
    inherits (access_type, bool virt);

    void
    set_left_node (class_& n)
    {
      derived_ = &n;
    }

    void
    set_right_node (class_& n)
    {
      base_ = &n;
    }

  protected:
    bool virt_;
    access_type access_;

    class_* base_;
    class_* derived_;
  };

  //
  //
  class class_: public virtual type, public scope
  {
  private:
    typedef std::vector<inherits*> inherits_list;

  public:
    typedef pointer_iterator<inherits_list::const_iterator> inherits_iterator;

    inherits_iterator
    inherits_begin () const
    {
      return inherits_.begin ();
    }

    inherits_iterator
    inherits_end () const
    {
      return inherits_.end ();
    }

  public:
    bool
    default_ctor () const;

    bool
    complete () const;

    bool
    abstract () const;

  public:
    // When doing lookup in class scope, take into account bases.
    //
    static unsigned int const exclude_base = 0x04;  // Exclude base classes.

    virtual names*
    lookup (string const& name,
            type_id const&,
            unsigned int flags = 0,
            bool* hidden = 0) const;

    using scope::lookup;

  public:
    class_ (path const&, size_t line, size_t column, tree);

    void
    add_edge_left (inherits& e)
    {
      inherits_.push_back (&e);
    }

    void
    add_edge_right (inherits&)
    {
    }

    using scope::add_edge_left;
    using type::add_edge_right;

    // Resolve conflict between scope::scope and nameable::scope.
    //
    using nameable::scope;

  protected:
    class_ ()
    {
    }

  private:
    inherits_list inherits_;
  };
}

#endif // ODB_SEMANTICS_CLASS_HXX
