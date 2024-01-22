// file      : odb/semantics/template.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_TEMPLATE_HXX
#define ODB_SEMANTICS_TEMPLATE_HXX

#include <vector>
#include <odb/semantics/elements.hxx>

namespace semantics
{
  //
  //
  class instantiates;

  class template_: public virtual nameable
  {
    typedef std::vector<instantiates*> instantiated;

  public:
    typedef
    pointer_iterator<instantiated::const_iterator>
    instantiated_iterator;

    instantiated_iterator
    instantiated_begin () const
    {
      return instantiated_.begin ();
    }

    instantiated_iterator
    instantiated_end () const
    {
      return instantiated_.end ();
    }

  public:
    void
    add_edge_right (instantiates& e)
    {
      instantiated_.push_back (&e);
    }

    using nameable::add_edge_right;

  protected:
    template_ ();

  private:
    instantiated instantiated_;
  };

  //
  //
  class instantiation;

  class instantiates: public edge
  {
  public:
    typedef semantics::template_ template_type;
    typedef semantics::instantiation instantiation_type;

    template_type&
    template_ () const
    {
      return *template__;
    }

    instantiation_type&
    instantiation () const
    {
      return *instantiation_;
    }

  public:
    instantiates ();

    void
    set_left_node (instantiation_type& n)
    {
      instantiation_ = &n;
    }

    void
    set_right_node (template_type& n)
    {
      template__ = &n;
    }

  private:
    template_type* template__;
    instantiation_type* instantiation_;
  };

  //
  //
  class instantiation: public virtual node
  {
  public:
    typedef semantics::template_ template_type;
    typedef semantics::instantiates instantiates_type;

    template_type&
    template_ () const
    {
      return instantiates_->template_ ();
    }

    instantiates_type&
    instantiates () const
    {
      return *instantiates_;
    }

  public:
    void
    add_edge_left (instantiates_type& e)
    {
      instantiates_ = &e;
    }

  protected:
    instantiation ();

  private:
    instantiates_type* instantiates_;
  };

  //
  // Type template and instantiation.
  //

  class type_template: public template_
  {
  protected:
    type_template ();
  };

  class type_instantiation: public virtual type, public instantiation
  {
  protected:
    type_instantiation ();
  };
}

#endif // ODB_SEMANTICS_TEMPLATE_HXX
