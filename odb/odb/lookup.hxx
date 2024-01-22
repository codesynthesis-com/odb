// file      : odb/lookup.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_LOOKUP_HXX
#define ODB_LOOKUP_HXX

#include <odb/gcc.hxx>

#include <string>

#include <odb/cxx-lexer.hxx>
#include <odb/semantics/elements.hxx>

namespace lookup
{
  struct invalid_name
  {
  };

  struct unable_to_resolve
  {
    unable_to_resolve (std::string const& n, bool l): name_ (n), last_ (l) {}

    std::string const&
    name () const {return name_;}

    // Last component in the name.
    //
    bool
    last () const {return last_;}

  private:
    std::string name_;
    bool last_;
  };

  std::string
  parse_scoped_name (cxx_lexer&,
                     cpp_ttype&,
                     std::string& tl, // Token literal.
                     tree& tn);       // Token node.

  // If trailing_scope is true, then this function also handles
  // names in the 'foo::bar::<something-other-than-name>' form.
  // In this case token will be <something-other-than-name> and
  // ptt will be CPP_SCOPE.
  //
  // The names are appended to the 'name' variable as they are
  // being resolved.
  //
  tree
  resolve_scoped_name (cxx_lexer&,
                       cpp_ttype&,
                       std::string& tl, // Token literal.
                       tree& tn,        // Token node.
                       cpp_ttype& ptt,  // Previous token type.
                       tree start_scope,
                       std::string& name,
                       bool is_type,
                       bool trailing_scope = false,
                       tree* end_scope = 0);

  // The same but using semantic graph instead of GCC tree. Also
  // throws semantics::unresolved instead of unable_to_resolve.
  //
  semantics::node&
  resolve_scoped_name (cxx_lexer&,
                       cpp_ttype&,
                       std::string& tl, // Token literal.
                       tree& tn,        // Token node.
                       cpp_ttype& ptt,  // Previous token type.
                       semantics::scope& start_scope,
                       std::string& name,
                       semantics::type_id const&,
                       bool trailing_scope = false,
                       semantics::scope** end_scope = 0);

  template <typename T>
  T&
  resolve_scoped_name (cxx_lexer& l,
                       cpp_ttype& tt,
                       std::string& tl, // Token literal.
                       tree& tn,        // Token node.
                       cpp_ttype& ptt,  // Previous token type.
                       semantics::scope& start_scope,
                       std::string& name,
                       bool trailing_scope = false,
                       semantics::scope** end_scope = 0)
  {
    return dynamic_cast<T&> (
      resolve_scoped_name (
        l, tt, tl, tn, ptt,
        start_scope, name, typeid (T), trailing_scope, end_scope));
  }
}

#endif // ODB_LOOKUP_HXX
