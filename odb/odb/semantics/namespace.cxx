// file      : odb/semantics/namespace.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/namespace.hxx>

namespace semantics
{
  namespace_::
  namespace_ (path const& file, size_t line, size_t column, tree tn)
      : node (file, line, column, tn), original_ (0)
  {
  }

  namespace_::
  namespace_ ()
      : original_ (0)
  {
  }

  names* namespace_::
  lookup (string const& name,
          type_id const& ti,
          unsigned int flags,
          bool* hidden) const
  {
    if (original_ != 0)
      return original_->lookup (name, ti, flags, hidden);

    // Being hidden in one namespace doesn't mean it is also hidden in
    // the other. Normally that would be an ambiguous lookup, but we use
    // relaxed rules.
    //
    bool h (false);     // Indicates whether this namespace hides the name.
    bool any_h (false); // Indicates whether any namespace hides the name.

    names* r (scope::lookup (name, ti, flags | exclude_outer, &h));
    any_h = any_h || h;

    if (r != 0 && h && hidden != 0)
      *hidden = true;

    for (extensions_iterator i (extensions_begin ());
         i != extensions_end ();
         ++i)
    {
      h = false;
      names* er ((*i)->scope::lookup (name, ti, flags | exclude_outer, &h));
      any_h = any_h || h;

      if (er != 0)
      {
        if (r != 0)
        {
          // If both are namespaces, then the one is just an extension
          // of the other.
          //
          if (!(r->named ().is_a<namespace_> () &&
                er->named ().is_a<namespace_> ()))
            throw ambiguous (*r, *er);
        }
        else
          r = er;

        if (h && hidden != 0)
          *hidden = true;
      }
    }

    if (r != 0)
      return r;

    if (any_h)
    {
      if (hidden != 0)
        *hidden = true;

      if ((flags & include_hidden) == 0)
        return 0;
    }

    // Look in the outer scope unless requested not to or if this is
    // the global scope.
    //
    if ((flags & exclude_outer) == 0 && !global_scope ())
      return scope ().lookup (name, ti, flags, hidden);

    return 0;
  }

  // type info
  //
  namespace
  {
    struct init
    {
      init ()
      {
        using compiler::type_info;

        type_info ti (typeid (namespace_));
        ti.add_base (typeid (scope));
        insert (ti);
      }
    } init_;
  }
}
