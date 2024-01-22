// file      : odb/lookup.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/lookup.hxx>
#include <odb/semantics.hxx>

using namespace std;

namespace lookup
{
  // Return canonical fundamental type name (<length> <sign> <type>, e.g.,
  // short unsigned int) or empty string if it is not a fundamental type.
  // Note that the type can still be invalid (e.g., unsigned float) so it
  // needs to be resolved.
  //
  static string
  parse_fundamental (cxx_lexer& l,
                     cpp_ttype& tt,
                     string& tl,
                     tree& tn,
                     cpp_ttype& ptt,
                     string& name)
  {
    bool
      si (false), // signed
      un (false), // unsigned
      sh (false), // short
      lo (false), // long
      ll (false); // long long

    string type;

    for (; tt == CPP_KEYWORD; ptt = tt, tt = l.next (tl, &tn))
    {
      if (!name.empty ())
        name += ' ';
      name += tl;

      if (tl == "signed")
      {
        if (si || un)
          throw invalid_name ();

        si = true;
      }
      else if (tl == "unsigned")
      {
        if (si || un)
          throw invalid_name ();

        un = true;
      }
      else if (tl == "short")
      {
        if (sh || lo || ll)
          throw invalid_name ();

        sh = true;
      }
      else if (tl == "long")
      {
        if (sh || ll)
          throw invalid_name ();

        if (lo)
        {
          lo = false;
          ll = true;
        }
        else
          lo = true;
      }
      else if (tl == "bool"     ||
               tl == "char"     ||
               tl == "wchar_t"  ||
               tl == "char16_t" || // C++11
               tl == "char32_t" || // C++11
               tl == "int"      ||
               tl == "float"    ||
               tl == "double")
      {
        if (!type.empty ())
          throw invalid_name ();

        type = tl;
      }
      else
        break;
    }

    if (type.empty () && (si || un || sh || lo || ll))
      type = "int"; // If type not specified, it defaults to int.

    if (type.empty ())
      return type;

    string r;

    if (sh)
      r += "short ";

    if (lo)
      r += "long ";

    if (ll)
      r += "long long ";

    if (si && type == "char")
      r += "signed ";

    if (un)
      r += "unsigned ";

    r += type;
    return r;
  }

  string
  parse_scoped_name (cxx_lexer& l, cpp_ttype& tt, string& tl, tree& tn)
  {
    string name;

    if (tt == CPP_SCOPE)
    {
      name += "::";
      tt = l.next (tl, &tn);
    }
    else if (tt == CPP_KEYWORD)
    {
      cpp_ttype ptt; // Not used.
      string t (parse_fundamental (l, tt, tl, tn, ptt, name));

      if (!t.empty ())
        return name;
    }

    while (true)
    {
      if (tt != CPP_NAME)
        throw invalid_name ();

      name += tl;
      tt = l.next (tl, &tn);

      if (tt != CPP_SCOPE)
        break;

      name += "::";
      tt = l.next (tl, &tn);
    }

    return name;
  }

  tree
  resolve_scoped_name (cxx_lexer& l,
                       cpp_ttype& tt,
                       string& tl,
                       tree& tn,
                       cpp_ttype& ptt,
                       tree scope,
                       string& name,
                       bool is_type,
                       bool trailing_scope,
                       tree* end_scope)
  {
    tree id;
    bool first (true);

    if (tt == CPP_SCOPE)
    {
      name += "::";
      scope = global_namespace;
      first = false;

      ptt = tt;
      tt = l.next (tl, &tn);
    }
    else if (tt == CPP_KEYWORD)
    {
      string t (parse_fundamental (l, tt, tl, tn, ptt, name));

      if (!t.empty ())
      {
        tree decl (
          lookup_qualified_name (
            global_namespace, get_identifier (t.c_str ()), true, false));

        if (decl == error_mark_node)
          throw unable_to_resolve (name, true);

        if (end_scope != 0)
          *end_scope = global_namespace;

        return decl;
      }
    }

    while (true)
    {
      if (end_scope != 0)
        *end_scope = scope;

      if (tt != CPP_NAME)
        throw invalid_name ();

      name += tl;
      id = get_identifier (tl.c_str ());
      ptt = tt;
      tt = l.next (tl, &tn);

      bool last (true);
      if (tt == CPP_SCOPE)
      {
        // If trailing scope names are allowed, then we also need to
        // check what's after the scope.
        //
        if (trailing_scope)
        {
          ptt = tt;
          tt = l.next (tl, &tn);

          if (tt == CPP_NAME)
            last = false;
        }
        else
          last = false;
      }

      tree decl = lookup_qualified_name (scope, id, last && is_type, false);

      // If this is the first component in the name, then also search the
      // outer scopes.
      //
      if (decl == error_mark_node && first && scope != global_namespace)
      {
        do
        {
          scope = TYPE_P (scope)
            ? CP_TYPE_CONTEXT (scope)
            : CP_DECL_CONTEXT (scope);
          decl = lookup_qualified_name (scope, id, last && is_type, false);
        } while (decl == error_mark_node && scope != global_namespace);
      }

      if (decl == error_mark_node)
        throw unable_to_resolve (name, last);

      scope = decl;

      if (last)
        break;

      first = false;

      if (TREE_CODE (scope) == TYPE_DECL)
        scope = TREE_TYPE (scope);

      name += "::";

      if (!trailing_scope)
      {
        ptt = tt;
        tt = l.next (tl, &tn);
      }
    }

    return scope;
  }

  semantics::node&
  resolve_scoped_name (cxx_lexer& l,
                       cpp_ttype& tt,
                       string& tl,
                       tree& tn,
                       cpp_ttype& ptt,
                       semantics::scope& start_scope,
                       string& name,
                       semantics::type_id const& tid,
                       bool trailing_scope,
                       semantics::scope** end_scope)
  {
    bool first (true);
    semantics::scope* scope (&start_scope);

    if (tt == CPP_SCOPE)
    {
      name += "::";
      for (; !scope->global_scope (); scope = &scope->scope_ ()) ;
      first = false;

      ptt = tt;
      tt = l.next (tl, &tn);
    }
    else if (tt == CPP_KEYWORD)
    {
      string t (parse_fundamental (l, tt, tl, tn, ptt, name));

      if (!t.empty ())
      {
        for (; !scope->global_scope (); scope = &scope->scope_ ()) ;

        if (end_scope != 0)
          *end_scope = scope;

        return scope->lookup<semantics::fund_type> (t);
      }
    }

    semantics::names* r;

    for (;;)
    {
      if (end_scope != 0)
        *end_scope = scope;

      if (tt != CPP_NAME)
        throw invalid_name ();

      name += tl;
      string n (tl);
      ptt = tt;
      tt = l.next (tl, &tn);

      bool last (true);
      if (tt == CPP_SCOPE)
      {
        // If trailing scope names are allowed, then we also need to
        // check what's after the scope.
        //
        if (trailing_scope)
        {
          ptt = tt;
          tt = l.next (tl, &tn);

          if (tt == CPP_NAME)
            last = false;
        }
        else
          last = false;
      }

      // If this is the first component in the name, then also search the
      // outer scopes.
      //
      bool hidden (false);
      r = scope->lookup (
        n,
        (last ? tid : typeid (semantics::scope)),
        (first ? 0 : semantics::scope::exclude_outer) |
        (last ? semantics::scope::include_hidden : 0),
        (last ? &hidden : 0));

      if (r == 0)
        throw semantics::unresolved (name, hidden);

      if (last)
        break;

      scope = &dynamic_cast<semantics::scope&> (r->named ());
      first = false;

      name += "::";

      if (!trailing_scope)
      {
        ptt = tt;
        tt = l.next (tl, &tn);
      }
    }

    return r->named ();
  }
}
