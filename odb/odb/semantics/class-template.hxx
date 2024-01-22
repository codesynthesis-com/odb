// file      : odb/semantics/class-template.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_CLASS_TEMPLATE_HXX
#define ODB_SEMANTICS_CLASS_TEMPLATE_HXX

#include <odb/semantics/elements.hxx>
#include <odb/semantics/class.hxx>
#include <odb/semantics/template.hxx>

namespace semantics
{
  class class_template: public type_template, public scope
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
    class_template (path const&, size_t line, size_t column, tree);

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
    using scope::add_edge_right;

    // Resolve conflict between scope::scope and nameable::scope.
    //
    using nameable::scope;

  private:
    inherits_list inherits_;
  };

  class class_instantiation: public class_, public type_instantiation
  {
  public:
    class_instantiation (path const&, size_t line, size_t column, tree);

    using class_::add_edge_left;
    using type_instantiation::add_edge_left;
  };
}

#endif // ODB_SEMANTICS_CLASS_TEMPLATE_HXX
