// file      : odb/semantics/derived.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_DERIVED_HXX
#define ODB_SEMANTICS_DERIVED_HXX

#include <odb/semantics/elements.hxx>

namespace semantics
{
  //
  // Derived types (cvr-qualifiers, pointer, reference, and array).
  //

  class derived_type: public type
  {
  public:
    virtual type&
    base_type () const = 0;
  };

  //
  //
  class qualifier;

  class qualifies: public edge
  {
  public:
    typedef semantics::type type_type;
    typedef semantics::qualifier qualifier_type;

    type_type&
    type () const
    {
      return *type_;
    }

    qualifier_type&
    qualifier () const
    {
      return *qualifier_;
    }

    // Name hint of the base type.
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
    qualifies ();

    void
    set_left_node (qualifier_type& n)
    {
      qualifier_ = &n;
    }

    void
    set_right_node (type_type& n)
    {
      type_ = &n;
    }

  protected:
    type_type* type_;
    qualifier_type* qualifier_;
    names* hint_;
  };

  class qualifier: public derived_type
  {
  public:
    typedef semantics::qualifies qualifies_type;

    bool
    const_ () const
    {
      return c_;
    }

    bool
    volatile_ () const
    {
      return v_;
    }

    bool
    restrict_ () const
    {
      return r_;
    }

    virtual type&
    base_type () const
    {
      return qualifies_->type ();
    }

    qualifies_type&
    qualifies () const
    {
      return *qualifies_;
    }

  public:
    virtual string
    fq_name (names*) const;

  public:
    qualifier (path const&,
               size_t line,
               size_t column,
               tree,
               bool c,
               bool v,
               bool r);

    void
    add_edge_left (qualifies_type& e)
    {
      qualifies_ = &e;
    }

  private:
    bool c_, v_, r_;
    qualifies_type* qualifies_;
  };

  //
  //
  class pointer;

  class points: public edge
  {
  public:
    typedef semantics::type type_type;
    typedef semantics::pointer pointer_type;

    type_type&
    type () const
    {
      return *type_;
    }

    pointer_type&
    pointer () const
    {
      return *pointer_;
    }

    // Name hint of the base type.
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
    points ();

    void
    set_left_node (pointer_type& n)
    {
      pointer_ = &n;
    }

    void
    set_right_node (type_type& n)
    {
      type_ = &n;
    }

  protected:
    type_type* type_;
    pointer_type* pointer_;
    names* hint_;
  };

  class pointer: public derived_type
  {
  public:
    typedef semantics::points points_type;

    virtual type&
    base_type () const
    {
      return points_->type ();
    }

    points_type&
    points () const
    {
      return *points_;
    }

  public:
    virtual string
    fq_name (names*) const;

  public:
    pointer (path const&, size_t line, size_t column, tree);

    void
    add_edge_left (points_type& e)
    {
      points_ = &e;
    }

  private:
    points_type* points_;
  };


  //
  //
  class reference;

  class references: public edge
  {
  public:
    typedef semantics::type type_type;
    typedef semantics::reference reference_type;

    type_type&
    type () const
    {
      return *type_;
    }

    reference_type&
    reference () const
    {
      return *reference_;
    }

    // Name hint of the base type.
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
    references ();

    void
    set_left_node (reference_type& n)
    {
      reference_ = &n;
    }

    void
    set_right_node (type_type& n)
    {
      type_ = &n;
    }

  protected:
    type_type* type_;
    reference_type* reference_;
    names* hint_;
  };

  class reference: public derived_type
  {
  public:
    typedef semantics::references references_type;

    virtual type&
    base_type () const
    {
      return references_->type ();
    }

    references_type&
    references () const
    {
      return *references_;
    }

  public:
    virtual string
    fq_name (names*) const;

  public:
    reference (path const&, size_t line, size_t column, tree);

    void
    add_edge_left (references_type& e)
    {
      references_ = &e;
    }

  private:
    references_type* references_;
  };


  //
  //
  class array;

  class contains: public edge
  {
  public:
    typedef semantics::type type_type;
    typedef semantics::array array_type;

    type_type&
    type () const
    {
      return *type_;
    }

    array_type&
    array () const
    {
      return *array_;
    }

    // Name hint of the base type.
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
    contains ();

    void
    set_left_node (array_type& n)
    {
      array_ = &n;
    }

    void
    set_right_node (type_type& n)
    {
      type_ = &n;
    }

  protected:
    type_type* type_;
    array_type* array_;
    names* hint_;
  };

  class array: public derived_type
  {
  public:
    typedef semantics::contains contains_type;

    // Return the number of elements in the array or 0 if it is not
    // specified (e.g., int[]).
    //
    unsigned long long
    size () const
    {
      return size_;
    }

    virtual type&
    base_type () const
    {
      return contains_->type ();
    }

    contains_type&
    contains () const
    {
      return *contains_;
    }

  public:
    virtual string
    fq_name (names*) const;

  private:
    friend class qualifier;

    string
    fq_name (names*, string& trailer) const;

    using derived_type::fq_name; // Unhide.

  public:
    array (path const&,
           size_t line,
           size_t column,
           tree,
           unsigned long long size);

    void
    add_edge_left (contains_type& e)
    {
      contains_ = &e;
    }

  private:
    contains_type* contains_;
    unsigned long long size_;
  };
}

#endif // ODB_SEMANTICS_DERIVED_HXX
