// file      : odb/semantics/namespace.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_NAMESPACE_HXX
#define ODB_SEMANTICS_NAMESPACE_HXX

#include <vector>

#include <odb/semantics/elements.hxx>

namespace semantics
{
  class namespace_: public scope
  {
    typedef std::vector<namespace_*> extensions_type;

  public:
    bool
    extension () const
    {
      return original_ != 0;
    }

    namespace_&
    original ()
    {
      return *original_;
    }

    void
    original (namespace_& ns)
    {
      original_ = &ns;
      ns.extensions_.push_back (this);
    }

  public:
    typedef extensions_type::const_iterator extensions_iterator;

    extensions_iterator
    extensions_begin () const {return extensions_.begin ();}

    extensions_iterator
    extensions_end () const {return extensions_.end ();}

  public:
    virtual names*
    lookup (string const& name,
            type_id const&,
            unsigned int flags = 0,
            bool* hidden = 0) const;

    using scope::lookup;

  public:
    namespace_ (path const&, size_t line, size_t column, tree);

    // Resolve conflict between scope::scope and nameable::scope.
    //
    using nameable::scope;

  protected:
    namespace_ ();

  private:
    namespace_* original_;
    extensions_type extensions_;
  };
}

#endif // ODB_SEMANTICS_NAMESPACE_HXX
