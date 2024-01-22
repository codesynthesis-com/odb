// file      : odb/semantics/enum.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_ENUM_HXX
#define ODB_SEMANTICS_ENUM_HXX

#include <vector>
#include <odb/semantics/elements.hxx>
#include <odb/semantics/fundamental.hxx>

namespace semantics
{
  class enum_;
  class enumerator;

  class enumerates: public edge
  {
  public:
    typedef semantics::enum_ enum_type;
    typedef semantics::enumerator enumerator_type;

    enum_type&
    enum_ () const
    {
      return *enum__;
    }

    enumerator_type&
    enumerator () const
    {
      return *enumerator_;
    }

  public:
    enumerates ();

    void
    set_left_node (enum_type& n)
    {
      enum__ = &n;
    }

    void
    set_right_node (enumerator_type& n)
    {
      enumerator_ = &n;
    }

  protected:
    enum_type* enum__;
    enumerator_type* enumerator_;
  };

  //
  //
  class enumerator: public nameable, public instance
  {
  public:
    typedef semantics::enum_ enum_type;

    enum_type&
    enum_ () const
    {
      return enumerated_->enum_ ();
    }

    enumerates&
    enumerated () const
    {
      return *enumerated_;
    }

    // If the enumeration is signed, then this value should be re-
    // interpreted as signed.
    //
    unsigned long long
    value () const
    {
      return value_;
    }

  public:
    enumerator (path const&,
                size_t line,
                size_t column,
                tree,
                unsigned long long value);

    void
    add_edge_right (enumerates& e)
    {
      enumerated_ = &e;
    }

    using nameable::add_edge_right;

  private:
    unsigned long long value_;
    enumerates* enumerated_;
  };

  //
  //
  class underlies: public edge
  {
  public:
    typedef semantics::enum_ enum_type;

    integral_type&
    type () const
    {
      return *type_;
    }

    enum_type&
    enum_ () const
    {
      return *enum__;
    }

    // Names edge in terms of which this edge was defined. Can be NULL.
    //
  public:
    void
    hint (names& hint)
    {
      hint_ = &hint;
    }

    names*
    hint () const
    {
      return hint_;
    }

  public:
    underlies ();

    void
    set_left_node (integral_type& n)
    {
      type_ = &n;
    }

    void
    set_right_node (enum_type& n)
    {
      enum__ = &n;
    }

  protected:
    integral_type* type_;
    enum_type* enum__;
    names* hint_;
  };

  //
  //
  class enum_: public type, public scope
  {
  private:
    typedef std::vector<enumerates*> enumerates_list;

  public:
    typedef
    pointer_iterator<enumerates_list::const_iterator>
    enumerates_iterator;

    enumerates_iterator
    enumerates_begin () const
    {
      return enumerates_.begin ();
    }

    enumerates_iterator
    enumerates_end () const
    {
      return enumerates_.end ();
    }

    underlies&
    underlied () const
    {
      return *underlied_;
    }

    integral_type&
    underlying_type () const
    {
      return underlied_->type ();
    }

    names*
    underlying_type_hint () const
    {
      return underlied_->hint ();
    }

    bool
    unsigned_ () const
    {
      return underlying_type ().unsigned_ ();
    }

  public:
    enum_ (path const&, size_t line, size_t column, tree);

    void
    add_edge_right (underlies& e)
    {
      underlied_ = &e;
    }

    void
    add_edge_left (enumerates& e)
    {
      enumerates_.push_back (&e);
    }

    using scope::add_edge_left;

  private:
    enumerates_list enumerates_;
    underlies* underlied_;
  };
}

#endif // ODB_SEMANTICS_ENUM_HXX
