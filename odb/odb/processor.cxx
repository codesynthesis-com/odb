// file      : odb/processor.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <iostream>
#include <algorithm> // std::find

#include <odb/common.hxx>
#include <odb/lookup.hxx>
#include <odb/context.hxx>
#include <odb/cxx-lexer.hxx>
#include <odb/processor.hxx>
#include <odb/diagnostics.hxx>

#include <odb/relational/processor.hxx>

using namespace std;

namespace
{
  // Find name hint for this type decl.
  //
  static semantics::names*
  find_hint (semantics::unit& u, tree decl)
  {
    semantics::names* r (0);

    for (tree ot (DECL_ORIGINAL_TYPE (decl));
         ot != 0;
         ot = decl ? DECL_ORIGINAL_TYPE (decl) : 0)
    {
      if ((r = u.find_hint (ot)))
        break;

      decl = TYPE_NAME (ot);
    }

    return r;
  }

  // Indirect (dynamic) context values.
  //
  static semantics::type*
  id_tree_type (semantics::names*& hint)
  {
    context& c (context::current ());
    data_member_path& id (*context::id_member (*c.top_object));
    return &c.utype (id, hint);
  }

  struct data_member: traversal::data_member, context
  {
    virtual void
    traverse (semantics::data_member& m)
    {
      if (transient (m))
        return;

      semantics::names* hint;
      semantics::type& t (utype (m, hint));

      // See if this member is a section.
      //
      if (t.fq_name () == "::odb::section")
      {
        using semantics::class_;

        class_& c (dynamic_cast<class_&> (m.scope ()));
        class_* poly_root (polymorphic (c));
        semantics::data_member* opt (optimistic (c));

        // If we have sections in a polymorphic optimistic hierarchy,
        // then the version member should be in the root.
        //
        if (poly_root == &c && opt != 0 && &opt->scope () != &c)
        {
          error (m.location ()) << "version must be a direct data member " <<
            "of a class that contains sections" << endl;
          info (opt->location ()) << "version member is declared here" << endl;
          throw operation_failed ();
        }

        process_user_section (m, c);

        // We don't need a modifier but the accessor should be by-reference.
        //
        process_access (m, "get");

        member_access& ma (m.get<member_access> ("get"));
        if (ma.by_value)
        {
          error (ma.loc) << "accessor returning a value cannot be used "
                         << "for a section" << endl;
          info (ma.loc) << "accessor returning a const reference is required"
                        << endl;
          info (m.location ()) << "data member is defined here" << endl;
          throw operation_failed ();
        }

        // Mark this member as transient since we don't store it in the
        // database.
        //
        m.set ("transient", true);

        features.section = true;
        return;
      }
      else
      {
        process_access (m, "get");
        process_access (m, "set");
      }

      // See if this member belongs to a section.
      //
      if (m.count ("section-member") != 0)
        process_section_member (m);

      // We don't need to do any further processing for common if we
      // are generating static multi-database code.
      //
      if (multi_static && options.database ()[0] == database::common)
        return;

      // Handle wrappers.
      //
      semantics::type* wt (0), *qwt (0);
      semantics::names* whint (0);
      if (process_wrapper (t))
      {
        qwt = t.get<semantics::type*> ("wrapper-type");
        whint = t.get<semantics::names*> ("wrapper-hint");
        wt = &utype (*qwt, whint);
      }

      // If the type is const and the member is not id, version, or
      // inverse, then mark it as readonly. In case of a wrapper,
      // both the wrapper type and the wrapped type must be const.
      // To see why, consider these possibilities:
      //
      // unique_ptr<const T> - can modify by setting a new pointer
      // const unique_ptr<T> - can modify by changing the pointed-to value
      //
      if (const_member (m) && !(id (m) || version (m) || m.count ("inverse")))
      {
        if (qwt == 0 || const_type (*qwt))
          m.set ("readonly", true);
      }

      process_points_to (m);

      if (composite_wrapper (t))
        return;

      // Process object pointer. The resulting column will be a simple
      // or composite value.
      //
      if (process_object_pointer (m, t))
        return;

      // Before checking if this is a container, check if this member
      // or its type were deduced to be a simple value based on the
      // pragmas. This is necessary because a container member (e.g.,
      // vector<char>) can be "overridden" into a simple value (e.g.,
      // BLOB) with a pragma.
      //
      if (m.count ("simple") ||
          t.count ("simple") ||
          (wt != 0 && wt->count ("simple")))
        return;

      process_container (m, (wt != 0 ? *wt : t));
    }

    //
    // Process member access expressions.
    //

    enum found_type
    {
      found_none,
      found_some, // Found something but keep looking for a better one.
      found_best
    };

    // Check if a function is a suitable accessor for this member.
    //
    found_type
    check_accessor (semantics::data_member& m,
                    tree f,
                    string const& n,
                    member_access& ma,
                    bool strict)
    {
      // Must be const.
      //
      if (!DECL_CONST_MEMFUNC_P (f))
        return found_none;

      // Accessor is a function with no arguments (other than 'this').
      //
      if (FUNCTION_FIRST_USER_PARMTYPE (f) != void_list_node)
        return found_none;

      // Note that to get the return type we have to use
      // TREE_TYPE(TREE_TYPE()) and not DECL_RESULT, as
      // suggested in the documentation.
      //
      tree r (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (f))));
      gcc_tree_code_type tc (TREE_CODE (r));

      // In the strict mode make sure the function returns for non-array
      // types a value or a (const) reference to the member type and for
      // array types a (const) pointer to element type. In the lax mode
      // we just check that the return value is not void.
      //
      if (strict)
      {
        semantics::type& t (utype (m));
        semantics::array* ar (dynamic_cast<semantics::array*> (&t));

        if (ar != 0 && tc != POINTER_TYPE)
          return found_none;

        tree bt (ar != 0 || tc == REFERENCE_TYPE ? TREE_TYPE (r) : r);
        tree bt_mv (TYPE_MAIN_VARIANT (bt));

        if ((ar != 0 ? ar->base_type () : t).tree_node () != bt_mv)
          return found_none;
      }
      else if (r == void_type_node)
        return found_none;

      cxx_tokens& e (ma.expr);
      e.push_back (cxx_token (0, CPP_KEYWORD, "this"));
      e.push_back (cxx_token (0, CPP_DOT));
      e.push_back (cxx_token (0, CPP_NAME, n));
      e.push_back (cxx_token (0, CPP_OPEN_PAREN));
      e.push_back (cxx_token (0, CPP_CLOSE_PAREN));

      // See if it returns by value.
      //
      ma.by_value = (tc != REFERENCE_TYPE && tc != POINTER_TYPE);

      return found_best;
    }

    // Check if a function is a suitable modifier for this member.
    //
    found_type
    check_modifier (semantics::data_member& m,
                    tree f,
                    string const& n,
                    member_access& ma,
                    bool strict)
    {
      tree a (FUNCTION_FIRST_USER_PARMTYPE (f));

      // For a modifier, it can either be a function that returns a non-
      // const reference (or non-const pointer, in case the member is an
      // array) or a by-value modifier that sets a new value. If both are
      // available, we prefer the former for efficiency.
      //
      cxx_tokens& e (ma.expr);
      semantics::type& t (utype (m));
      semantics::array* ar (dynamic_cast<semantics::array*> (&t));

      if (a == void_list_node)
      {
        // Note that to get the return type we have to use
        // TREE_TYPE(TREE_TYPE()) and not DECL_RESULT, as
        // suggested in the documentation.
        //
        tree r (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (f))));
        gcc_tree_code_type tc (TREE_CODE (r));

        // By-reference modifier. Should return a reference or a pointer.
        //
        if (tc != (ar != 0 ? POINTER_TYPE : REFERENCE_TYPE))
          return found_none;

        // The base type should not be const and, in strict mode, should
        // match the member type.
        //
        tree bt (TREE_TYPE (r));

        if (CP_TYPE_CONST_P (bt))
          return found_none;

        tree bt_mv (TYPE_MAIN_VARIANT (bt));

        if (strict && (ar != 0 ? ar->base_type () : t).tree_node () != bt_mv)
          return found_none;

        e.clear (); // Could contain by value modifier.
        e.push_back (cxx_token (0, CPP_KEYWORD, "this"));
        e.push_back (cxx_token (0, CPP_DOT));
        e.push_back (cxx_token (0, CPP_NAME, n));
        e.push_back (cxx_token (0, CPP_OPEN_PAREN));
        e.push_back (cxx_token (0, CPP_CLOSE_PAREN));

        return found_best;
      }
      // Otherwise look for a by value modifier, which is a function
      // with a single argument.
      //
      else if (TREE_CHAIN (a) == void_list_node)
      {
        // In the lax mode any function with a single argument works
        // for us. And we don't care what it returns.
        //
        if (strict)
        {
          // In the strict mode make sure the argument matches the
          // member. This is exactly the same logic as in accessor
          // with regards to arrays, references, etc.
          //
          tree at (TREE_VALUE (a));
          gcc_tree_code_type tc (TREE_CODE (at));

          if (ar != 0 && tc != POINTER_TYPE)
            return found_none;

          tree bt (ar != 0 || tc == REFERENCE_TYPE ? TREE_TYPE (at) : at);
          tree bt_mv (TYPE_MAIN_VARIANT (bt));

          if ((ar != 0 ? ar->base_type () : t).tree_node () != bt_mv)
            return found_none;
        }

        if (e.empty ())
        {
          e.push_back (cxx_token (0, CPP_KEYWORD, "this"));
          e.push_back (cxx_token (0, CPP_DOT));
          e.push_back (cxx_token (0, CPP_NAME, n));
          e.push_back (cxx_token (0, CPP_OPEN_PAREN));
          e.push_back (cxx_token (0, CPP_QUERY));
          e.push_back (cxx_token (0, CPP_CLOSE_PAREN));

          // Continue searching in case there is version that returns a
          // non-const reference which we prefer for efficiency.
          //
          return found_some;
        }
        else
          return found_none; // We didn't find anything better.
      }

      return found_none;
    }

    void
    process_access (semantics::data_member& m, std::string const& k)
    {
      bool virt (m.count ("virtual"));

      // Ignore certain special virtual members.
      //
      if (virt && (m.count ("polymorphic-ref") || m.count ("discriminator")))
        return;

      char const* kind (k == "get" ? "accessor" : "modifier");
      semantics::class_& c (dynamic_cast<semantics::class_&> (m.scope ()));

      // If we don't have an access expression, try to come up with
      // one.
      //
      if (!m.count (k))
      {
        found_type found (found_none);
        semantics::access const& a (m.named ().access ());
        member_access& ma (
          m.set (
            k, member_access (m.location (), kind, true)));

        // If this member is not virtual and is either public or if we
        // are a friend of this class, then go for the member directly.
        //
        if (!virt && (a == semantics::access::public_ ||
                      c.get<bool> ("friend")))
        {
          ma.expr.push_back (cxx_token (0, CPP_KEYWORD, "this"));
          ma.expr.push_back (cxx_token (0, CPP_DOT));
          ma.expr.push_back (cxx_token (0, CPP_NAME, m.name ()));
          found = found_best;
        }

        // Otherwise, try to find a suitable accessor/modifier.
        //

        // First try the original name. If that doesn't produce anything,
        // then try the public name.
        //
        bool t (k == "get"
                ? options.accessor_regex_trace ()
                : options.modifier_regex_trace ());
        regex_mapping const& re (
          k == "get" ? accessor_regex : modifier_regex);

        for (unsigned short j (0); found != found_best && j != 2; ++j)
        {
          string b (j == 0 ? m.name () : public_name (m, false));

          // Skip the second pass if original and public names are the same.
          //
          if (j == 1 && b == m.name ())
            continue;

          if (t)
            cerr << kind << (j == 0 ? " original" : " public")
                 << " name '" << b << "'" << endl;

          for (regex_mapping::const_iterator i (re.begin ());
               found != found_best && i != re.end ();
               ++i)
          {
            if (t)
              cerr << "try: '" << i->regex () << "' : ";

            if (!i->match (b))
            {
              if (t)
                cerr << '-' << endl;
              continue;
            }

            string n (i->replace (b));

            if (t)
              cerr << "'" << n << "' : ";

            tree decl (
              lookup_qualified_name (
                c.tree_node (), get_identifier (n.c_str ()), false, false));

            if (decl == error_mark_node || TREE_CODE (decl) != BASELINK)
            {
              if (t)
                cerr << '-' << endl;
              continue;
            }

            // OVL_* macros work for both FUNCTION_DECL and OVERLOAD.
            //
#if BUILDING_GCC_MAJOR >= 8
            for (ovl_iterator i (BASELINK_FUNCTIONS (decl)); i; ++i)
#else
            for (tree o (BASELINK_FUNCTIONS (decl)); o != 0; o = OVL_NEXT (o))
#endif
            {
#if BUILDING_GCC_MAJOR >= 8
              tree f (*i);
#else
              tree f (OVL_CURRENT (o));
#endif
              // We are only interested in public non-static member
              // functions. Note that TREE_PUBLIC() returns something
              // other than what we need.
              //
              if (
#if BUILDING_GCC_MAJOR >= 14
                !DECL_OBJECT_MEMBER_FUNCTION_P (f)
#else
                !DECL_NONSTATIC_MEMBER_FUNCTION_P (f)
#endif
                  || TREE_PRIVATE (f)
                  || TREE_PROTECTED (f))
                continue;

              found_type r (k == "get"
                            ? check_accessor (m, f, n, ma, true)
                            : check_modifier (m, f, n, ma, true));

              if (r != found_none)
              {
                // Update the location of the access expression to point
                // to this function.
                //
                ma.loc = location (real_source_location (f));
                found = r;
              }
            }

            if (t)
              cerr << (found != found_none ? '+' : '-') << endl;
          }
        }

        // If that didn't work then the generated code won't be able
        // to access this member.
        //
        if (found == found_none)
        {
          location const& l (m.location ());

          if (virt)
          {
            error (l) << "no suitable " << kind << " function could be "
                      << "automatically found for virtual data member '"
                      << m.name () << "'" << endl;

            info (l)  << "use '#pragma db " << k << "' to explicitly "
                      << "specify the " << kind << " function or "
                      << "expression" << endl;
          }
          else
          {
            error (l) << "data member '" << m.name () << "' is "
                      << a.string () << " and no suitable " << kind
                      << " function could be automatically found" << endl;

            info (l)  << "consider making class 'odb::access' a friend of "
                      << "class '" << class_name (c) << "'" << endl;

            info (l)  << "or use '#pragma db " << k << "' to explicitly "
                      << "specify the " << kind << " function or "
                      << "expression" << endl;
          }

          throw operation_failed ();
        }
      }

      member_access& ma (m.get<member_access> (k));

      if (ma.empty ())
        return;

      cxx_tokens& e (ma.expr);

      // If it is just a name, resolve it and convert to an appropriate
      // expression.
      //
      if (e.size () == 1 && e.back ().type == CPP_NAME)
      {
        string n (e.back ().literal);
        e.clear ();

        tree decl (
          lookup_qualified_name (
            c.tree_node (), get_identifier (n.c_str ()), false, false));

        if (decl == error_mark_node)
        {
          error (ma.loc) << "unable to resolve data member or function "
                         << "name '" << n << "'" << endl;
          throw operation_failed ();
        }

        switch (TREE_CODE (decl))
        {
        case FIELD_DECL:
          {
            e.push_back (cxx_token (0, CPP_KEYWORD, "this"));
            e.push_back (cxx_token (0, CPP_DOT));
            e.push_back (cxx_token (0, CPP_NAME, n));
            break;
          }
        case BASELINK:
          {
            // OVL_* macros work for both FUNCTION_DECL and OVERLOAD.
            //
#if BUILDING_GCC_MAJOR >= 8
            for (ovl_iterator i (BASELINK_FUNCTIONS (decl)); i; ++i)
#else
            for (tree o (BASELINK_FUNCTIONS (decl)); o != 0; o = OVL_NEXT (o))
#endif
            {
#if BUILDING_GCC_MAJOR >= 8
              tree f (*i);
#else
              tree f (OVL_CURRENT (o));
#endif
              // We are only interested in non-static member functions.
              //
              if (
#if BUILDING_GCC_MAJOR >= 14
                !DECL_OBJECT_MEMBER_FUNCTION_P (f)
#else
                !DECL_NONSTATIC_MEMBER_FUNCTION_P (f)
#endif
              )
                continue;

              if ((k == "get"
                   ? check_accessor (m, f, n, ma, false)
                   : check_modifier (m, f, n, ma, false)) == found_best)
                break;
            }

            if (e.empty ())
            {
              error (ma.loc) << "unable to find suitable " << kind
                             << " function '" << n << "'" << endl;
              throw operation_failed ();
            }
            break;
          }
        default:
          {
            error (ma.loc) << "name '" << n << "' does not refer to a data "
                           << "member or function" << endl;
            throw operation_failed ();
          }
        }
      }

      // If there is no 'this' keyword, then add it as a prefix.
      //
      {
        bool t (false);
        for (cxx_tokens::iterator i (e.begin ()); i != e.end (); ++i)
        {
          if (i->type == CPP_KEYWORD && i->literal == "this")
          {
            t = true;
            break;
          }
        }

        if (!t)
        {
          e.insert (e.begin (), cxx_token (0, CPP_DOT));
          e.insert (e.begin (), cxx_token (0, CPP_KEYWORD, "this"));
        }
      }

      // Check that there is no placeholder in the accessor expression.
      //
      if (k == "get" && ma.placeholder ())
      {
        error (ma.loc) << "(?) placeholder in the accessor expression"
                       << endl;
        throw operation_failed ();
      }

      // Check that the member type is default-constructible if we
      // have a by value modifier.
      //
      if (k == "set" && ma.placeholder ())
      {
        semantics::class_* c (dynamic_cast<semantics::class_*> (&utype (m)));

        // Assume all other types are default-constructible.
        //
        if (c != 0)
        {
          // If this type is a class template instantiation, then make
          // sure it is instantiated. While types used in real members
          // will be instantiated, this is not necessarily the case for
          // virtual members. Without the instantiation we won't be able
          // to detect whether the type has the default ctor.
          //
          // It would have been cleaner to do it in post_process_pragmas()
          // but there we don't yet know whether we need the default ctor.
          // And it is a good idea not to require instantiability unless
          // we really need it.
          //
          tree type (c->tree_node ());

          if (!COMPLETE_TYPE_P (type) &&
              CLASSTYPE_TEMPLATE_INSTANTIATION (type))
          {
            // Reset input location so that we get nice diagnostics in
            // case of an error. Use the location of the virtual pragma.
            //
            location_t loc (m.get<location_t> ("virtual-location"));
            input_location = loc;

            if (instantiate_class_template (type) == error_mark_node ||
                errorcount != 0 ||
                !COMPLETE_TYPE_P (type))
            {
              error (loc) << "unable to instantiate virtual data member " <<
                "type" << endl;
              throw operation_failed ();
            }
          }

          if (!c->default_ctor ())
          {
            error (ma.loc) << "modifier expression requires member type " <<
              "to be default-constructible" << endl;
            throw operation_failed ();
          }
        }
      }
    }

    //
    // Process section.
    //

    user_section&
    process_user_section (semantics::data_member& m, semantics::class_& c)
    {
      user_sections& uss (c.get<user_sections> ("user-sections"));

      user_section::load_type l (
        m.get ("section-load", user_section::load_eager));

      user_section::update_type u (
        m.get ("section-update", user_section::update_always));

      if (l == user_section::load_eager && u == user_section::update_always)
      {
        location const& l (m.location ());

        error (l) << "eager-loaded, always-updated section is pointless"
                  << endl;

        info (l) << "use '#pragma db load' and/or '#pragma db update' to "
          "specify an alternative loading and/or updating strategy" << endl;

        info (l) << "or remove the section altogether" << endl;

        throw operation_failed ();
      }

      size_t n (uss.count (user_sections::count_total |
                           user_sections::count_all   |
                           user_sections::count_special_version));
      user_section us (m, c, n, l, u);

      // We may already have seen this section (e.g., forward reference
      // from a member of this section).
      //
      user_sections::iterator i (find (uss.begin (), uss.end (), us));

      if (i != uss.end ())
        return *i;

      // If we are adding a new section to an optimistic class with
      // version in a base, make sure the base is sectionable.
      //
      semantics::data_member* opt (optimistic (c));
      if (opt != 0 && &opt->scope () != &c)
      {
        semantics::class_* poly_root (polymorphic (c));
        semantics::node* base (poly_root ? poly_root : &opt->scope ());

        if (!base->count ("sectionable"))
        {
          error (m.location ()) << "adding new section to a derived class " <<
            "in an optimistic hierarchy requires sectionable base class" <<
            endl;

          info (base->location ()) << "use '#pragma db object sectionable' " <<
            "to make the base class of this hierarchy sectionable" << endl;

          throw operation_failed ();
        }
      }

      uss.push_back (us);
      return uss.back ();
    }

    void
    process_section_member (semantics::data_member& m)
    {
      using semantics::class_;
      using semantics::data_member;

      string name (m.get<string> ("section-member"));
      location_t loc (m.get<location_t> ("section-member-location"));
      class_& c (dynamic_cast<class_&> (m.scope ()));

      class_* poly_root (polymorphic (c));
      bool poly_derived (poly_root != 0 && poly_root != &c);

      try
      {
        data_member& us (c.lookup<data_member> (name, class_::include_hidden));

        // Make sure we are referencing a section.
        //
        if (utype (us).fq_name () != "::odb::section")
        {
          error (loc) << "data member '" << name << "' in '#pragma db " <<
            "section' is not of the odb::section type" << endl;
          throw operation_failed ();
        }

        // If the section is in the base, handle polymorphic inheritance.
        //
        class_& b (dynamic_cast<class_&> (us.scope ()));
        user_section* s (0);

        if (&c != &b && poly_derived)
        {
          user_sections& uss (c.get<user_sections> ("user-sections"));

          // This is a section override. See if we have already handled
          // this section.
          //
          for (user_sections::iterator i (uss.begin ());
               s == 0 && i != uss.end ();
               ++i)
          {
            if (i->member == &us)
              s = &*i;
          }

          // Otherwise, find and copy the nearest override in the base.
          // The result should be a chain of overrides leading all the
          // way to the original section.
          //
          if (s == 0)
          {
            for (class_* b (&polymorphic_base (c));;
                 b = &polymorphic_base (*b))
            {
              user_sections& buss (b->get<user_sections> ("user-sections"));

              for (user_sections::iterator i (buss.begin ());
                   s == 0 && i != buss.end ();
                   ++i)
              {
                if (i->member == &us)
                {
                  uss.push_back (*i);
                  uss.back ().object = &c;
                  uss.back ().base = &*i;
                  s = &uss.back ();
                }
              }

              if (s != 0)
                break;

              assert (b != poly_root); // We should have found it by now.
            }
          }
        }
        else
          s = &process_user_section (us, c);

        // Mark the member as added/deleted if the section is added/deleted.
        // Also check that the version ordering is correct.
        //
        if (unsigned long long sav = added (*s->member))
        {
          location_t sl (s->member->get<location_t> ("added-location"));

          if (unsigned long long mav = added (m))
          {
            location_t ml (m.get<location_t> ("added-location"));

            if (mav < sav)
            {
              error (ml) << "member addition version is less than the " <<
                "section addition version" << endl;
              info (sl) << "section addition version is specified here" <<
                endl;
              throw operation_failed ();
            }

            if (mav == sav)
            {
              error (ml) << "member addition version is the same as " <<
                "section addition version" << endl;
              info (sl) << "section addition version is specified here" <<
                endl;
              info (ml) << "delete this pragma" << endl;
              throw operation_failed ();
            }
          }
          else
          {
            m.set ("added", sav);
            m.set ("added-location", sl);
          }
        }

        if (unsigned long long sdv = deleted (*s->member))
        {
          location_t sl (s->member->get<location_t> ("deleted-location"));

          if (unsigned long long mdv = deleted (m))
          {
            location_t ml (m.get<location_t> ("deleted-location"));

            if (mdv > sdv)
            {
              error (ml) << "member deletion version is greater than the " <<
                "section deletion version" << endl;
              info (sl) << "section deletion version is specified here" <<
                endl;
              throw operation_failed ();
            }

            if (mdv == sdv)
            {
              error (ml) << "member deletion version is the same as " <<
                "section deletion version" << endl;
              info (sl) << "section deletion version is specified here" <<
                endl;
              info (ml) << "delete this pragma" << endl;
              throw operation_failed ();
            }
          }
          else
          {
            m.set ("deleted", sdv);
            m.set ("deleted-location", sl);
          }
        }

        // Insert as object_section.
        //
        m.set ("section", static_cast<object_section*> (s));
      }
      catch (semantics::unresolved const& e)
      {
        if (e.type_mismatch)
          error (loc) << "name '" << name << "' in '#pragma db section' " <<
            "does not refer to a data member" << endl;
        else
          error (loc) << "unable to resolve data member '" << name << "' " <<
            "specified with '#pragma db section'" << endl;

        throw operation_failed ();
      }
      catch (semantics::ambiguous const& e)
      {
        error (loc) << "data member name '" << name << "' specified " <<
          "with '#pragma db section' is ambiguous" << endl;

        info (e.first.named ().location ()) << "could resolve to this " <<
          "data member" << endl;

        info (e.second.named ().location ()) << "or could resolve to " <<
          "this data member" << endl;

        throw operation_failed ();
      }
    }

    //
    // Process wrapper.
    //

    bool
    process_wrapper (semantics::type& t)
    {
      if (t.count ("wrapper"))
        return t.get<bool> ("wrapper");

      // Check this type with wrapper_traits.
      //
      tree inst (instantiate_template (wrapper_traits_, t.tree_node ()));

      if (inst == 0)
      {
        t.set ("wrapper", false);
        return false;
      }

      // @@ This points to the primary template, not the specialization.
      //
      tree decl (TYPE_NAME (inst));

      string f (DECL_SOURCE_FILE (decl));
      size_t l (DECL_SOURCE_LINE (decl));
      size_t c (DECL_SOURCE_COLUMN (decl));

      // Get the wrapped type.
      //
      try
      {
        tree decl (
          lookup_qualified_name (
            inst, get_identifier ("wrapped_type"), true, false));

        if (decl == error_mark_node || TREE_CODE (decl) != TYPE_DECL)
          throw operation_failed ();

        // The wrapped_type alias is a typedef in an instantiation
        // that we just instantiated dynamically. As a result there
        // is no semantic graph edges corresponding to this typedef
        // since we haven't parsed it yet (unless it was instantiated
        // explicitly by the user; see below). So to get the tree node
        // that can actually be resolved to the graph node, we use
        // the source type of this typedef.
        //
        tree type (DECL_ORIGINAL_TYPE (decl));

        bool qc (CP_TYPE_CONST_P (type));
        bool qv (CP_TYPE_VOLATILE_P (type));
        bool qr (CP_TYPE_RESTRICT_P (type));

        type = TYPE_MAIN_VARIANT (type);
        semantics::type* wt (
          dynamic_cast<semantics::type*> (unit.find (type)));

        // Object pointers and wrappers often use the same smart
        // pointers so check if the wrapped type is an object.
        //
        if (object (*wt))
        {
          t.set ("wrapper", false);
          return false;
        }

        if (qc || qv || qr)
        {
          for (semantics::type::qualified_iterator i (wt->qualified_begin ());
               i != wt->qualified_end (); ++i)
          {
            semantics::qualifier& q (i->qualifier ());

            if (q.const_ () == qc &&
                q.volatile_ () == qv &&
                q.restrict_ () == qr)
            {
              wt = &q;
              break;
            }
          }
        }

        // Find the hint.
        //
        // If we can't find any, then try to fallback to the wrapped_type
        // alias inside wrapper_traits. This requires an explicit
        // wrapper_traits instantiation (see above).
        //
        semantics::names* wh (find_hint (unit, decl));

        if (wh == nullptr)
          wh = unit.find_hint (TREE_TYPE (decl));

        t.set ("wrapper-type", wt);
        t.set ("wrapper-hint", wh);
      }
      catch (operation_failed const&)
      {
        os << f << ":" << l << ":" << c << ": error: "
           << "wrapper_traits specialization does not define the "
           << "wrapped_type type" << endl;
        throw;
      }

      // Get the null_handler flag.
      //
      bool null_handler (false);

      try
      {
        tree nh (
          lookup_qualified_name (
            inst, get_identifier ("null_handler"), false, false));

        if (nh == error_mark_node || TREE_CODE (nh) != VAR_DECL)
          throw operation_failed ();

        // Instantiate this decalaration so that we can get its value.
        //
        if (DECL_TEMPLATE_INSTANTIATION (nh) &&
            !DECL_TEMPLATE_INSTANTIATED (nh) &&
            !DECL_EXPLICIT_INSTANTIATION (nh))
          instantiate_decl (nh, false, false);

        tree init (DECL_INITIAL (nh));

        if (init == error_mark_node || TREE_CODE (init) != INTEGER_CST)
          throw operation_failed ();

        null_handler = static_cast<bool> (integer_value (init));
        t.set ("wrapper-null-handler", null_handler);
      }
      catch (operation_failed const&)
      {
        os << f << ":" << l << ":" << c << ": error: "
           << "wrapper_traits specialization does not define the "
           << "null_handler constant" << endl;
        throw;
      }

      // Get the null_default flag.
      //
      if (null_handler)
      {
        try
        {
          tree nh (
            lookup_qualified_name (
              inst, get_identifier ("null_default"), false, false));

          if (nh == error_mark_node || TREE_CODE (nh) != VAR_DECL)
            throw operation_failed ();

          // Instantiate this decalaration so that we can get its value.
          //
          if (DECL_TEMPLATE_INSTANTIATION (nh) &&
              !DECL_TEMPLATE_INSTANTIATED (nh) &&
              !DECL_EXPLICIT_INSTANTIATION (nh))
            instantiate_decl (nh, false, false);

          tree init (DECL_INITIAL (nh));

          if (init == error_mark_node || TREE_CODE (init) != INTEGER_CST)
            throw operation_failed ();

          t.set ("wrapper-null-default",
                 static_cast<bool> (integer_value (init)));
        }
        catch (operation_failed const&)
        {
          os << f << ":" << l << ":" << c << ": error: "
             << "wrapper_traits specialization does not define the "
             << "null_default constant" << endl;
          throw;
        }
      }

      // Check if the wrapper is a TR1 template instantiation.
      //
      if (tree ti = TYPE_TEMPLATE_INFO (t.tree_node ()))
      {
        tree decl (TI_TEMPLATE (ti)); // DECL_TEMPLATE

        // Get to the most general template declaration.
        //
        while (DECL_TEMPLATE_INFO (decl))
          decl = DECL_TI_TEMPLATE (decl);

        bool& tr1 (features.tr1_pointer);
        bool& boost (features.boost_pointer);

        string n (decl_as_string (decl, TFF_PLAIN_IDENTIFIER));

        // In case of a boost TR1 implementation, we cannot distinguish
        // between the boost:: and std::tr1:: usage since the latter is
        // just a using-declaration for the former.
        //
        tr1 = tr1
          || n.compare (0, 8, "std::tr1") == 0
          || n.compare (0, 10, "::std::tr1") == 0;

        boost = boost
          || n.compare (0, 17, "boost::shared_ptr") == 0
          || n.compare (0, 19, "::boost::shared_ptr") == 0;
      }

      t.set ("wrapper", true);
      return true;
    }

    //
    // Process object pointer.
    //

    semantics::class_*
    process_object_pointer (semantics::data_member& m,
                            semantics::type& t,
                            string const& kp = string ())
    {
      using semantics::class_;
      using semantics::data_member;

      class_* c (0);

      // The overall idea is as follows: try to instantiate the pointer
      // traits class template. If we are successeful, then get the
      // element type and see if it is an object.
      //
      if (t.count ("element-type"))
        c = t.get<class_*> ("element-type");
      else
      {
        tree inst (instantiate_template (pointer_traits_, t.tree_node ()));

        if (inst == 0)
          return 0;

        // @@ This points to the primary template, not the specialization.
        //
        tree decl (TYPE_NAME (inst));

        string fl (DECL_SOURCE_FILE (decl));
        size_t ln (DECL_SOURCE_LINE (decl));
        size_t cl (DECL_SOURCE_COLUMN (decl));

        // Get the element type.
        //
        tree tn (0);
        try
        {
          tree decl (
            lookup_qualified_name (
              inst, get_identifier ("element_type"), true, false));

          if (decl == error_mark_node || TREE_CODE (decl) != TYPE_DECL)
            throw operation_failed ();

          tn = TYPE_MAIN_VARIANT (TREE_TYPE (decl));

          // Check if the pointer is a TR1 template instantiation.
          //
          if (tree ti = TYPE_TEMPLATE_INFO (t.tree_node ()))
          {
            decl = TI_TEMPLATE (ti); // DECL_TEMPLATE

            // Get to the most general template declaration.
            //
            while (DECL_TEMPLATE_INFO (decl))
              decl = DECL_TI_TEMPLATE (decl);

            bool& tr1 (features.tr1_pointer);
            bool& boost (features.boost_pointer);

            string n (decl_as_string (decl, TFF_PLAIN_IDENTIFIER));

            // In case of a boost TR1 implementation, we cannot distinguish
            // between the boost:: and std::tr1:: usage since the latter is
            // just a using-declaration for the former.
            //
            tr1 = tr1
              || n.compare (0, 8, "std::tr1") == 0
              || n.compare (0, 10, "::std::tr1") == 0;

            boost = boost
              || n.compare (0, 17, "boost::shared_ptr") == 0
              || n.compare (0, 19, "::boost::shared_ptr") == 0;
          }
        }
        catch (operation_failed const&)
        {
          os << fl << ":" << ln << ":" << cl << ": error: pointer_traits "
             << "specialization does not define the 'element_type' type"
             << endl;
          throw;
        }

        c = dynamic_cast<class_*> (unit.find (tn));

        if (c == 0 || !object (*c))
          return 0;

        t.set ("element-type", c);

        // Determine the pointer kind.
        //
        try
        {
          tree kind (
            lookup_qualified_name (
              inst, get_identifier ("kind"), false, false));

          if (kind == error_mark_node || TREE_CODE (kind) != VAR_DECL)
            throw operation_failed ();

          // Instantiate this decalaration so that we can get its value.
          //
          if (DECL_TEMPLATE_INSTANTIATION (kind) &&
              !DECL_TEMPLATE_INSTANTIATED (kind) &&
              !DECL_EXPLICIT_INSTANTIATION (kind))
            instantiate_decl (kind, false, false);

          tree init (DECL_INITIAL (kind));

          if (init == error_mark_node || TREE_CODE (init) != INTEGER_CST)
            throw operation_failed ();

          pointer_kind_type pk = static_cast<pointer_kind_type> (
            integer_value (init));
          t.set ("pointer-kind", pk);
        }
        catch (operation_failed const&)
        {
          os << fl << ":" << ln << ":" << cl << ": error: pointer_traits "
             << "specialization does not define the 'kind' constant" << endl;
          throw;
        }

        // Get the lazy flag.
        //
        try
        {
          tree lazy (
            lookup_qualified_name (
              inst, get_identifier ("lazy"), false, false));

          if (lazy == error_mark_node || TREE_CODE (lazy) != VAR_DECL)
            throw operation_failed ();

          // Instantiate this decalaration so that we can get its value.
          //
          if (DECL_TEMPLATE_INSTANTIATION (lazy) &&
              !DECL_TEMPLATE_INSTANTIATED (lazy) &&
              !DECL_EXPLICIT_INSTANTIATION (lazy))
            instantiate_decl (lazy, false, false);

          tree init (DECL_INITIAL (lazy));

          if (init == error_mark_node || TREE_CODE (init) != INTEGER_CST)
            throw operation_failed ();

          t.set ("pointer-lazy", static_cast<bool> (integer_value (init)));
        }
        catch (operation_failed const&)
        {
          os << fl << ":" << ln << ":" << cl << ": error: pointer_traits "
             << "specialization does not define the 'kind' constant" << endl;
          throw;
        }
      }

      // See if this is the inverse side of a bidirectional relationship.
      // If so, then resolve the member path and cache it in the context.
      //
      if (m.count ("inverse"))
      {
        string name (m.get<string> ("inverse"));
        location_t l (m.get<location_t> ("inverse-location"));
        data_member_path mp (resolve_data_members (*c, name, l, lex_));

        {
          string tl;
          data_member& m (*mp.back ());

          if (container (m) && lex_.next (tl) != CPP_EOF)
          {
            error (l) << "unexpect name after container member " <<
              m.name () << " in '#pragma db inverse'" << endl;
            throw operation_failed ();
          }
        }

        // Validate each member.
        //
        for (data_member_path::iterator i (mp.begin ()); i != mp.end (); ++i)
        {
          data_member& im (**i);
          const string& n (im.name ());

          if (im.count ("transient"))
          {
            error (l) << "data member '" << n << "' specified with " <<
              "'#pragma db inverse' is transient" << endl;
            info (im.location ()) << "data member '" << n << "' is " <<
              "defined here" << endl;
            throw operation_failed ();
          }

          if (im.count ("inverse") || im.count ("value-inverse"))
          {
            error (l) << "data member '" << n << "' specified with " <<
              "'#pragma db inverse' is itself inverse" << endl;
            info (im.location ()) << "data member '" << n << "' is " <<
              "defined here" << endl;
            throw operation_failed ();
          }
        }

        // @@ Would be good to check that the other end is actually
        // an object pointer/points_to and points to the correct
        // object. But the other class may not have been processed
        // yet. Need to do in validator, pass 2.
        //
        m.remove ("inverse");
        m.set (kp + (kp.empty () ? "": "-") + "inverse", mp);
      }

      return c;
    }

    //
    // Process points-to pragma.
    //

    void
    process_points_to (semantics::data_member& m,
                       string const& /*kp*/ = string ())
    {
      if (!m.count ("points-to"))
        return;

      using semantics::class_;

      tree t (m.get<tree> ("points-to"));
      location_t l (m.get<location_t> ("points-to-location"));

      class_* c (dynamic_cast<class_*> (unit.find (t)));

      if (c == 0 || !object (*c))
      {
        error (l) << "name specified with '#pragma db points_to' does "
                  << "not refer to an object" << endl;
        throw operation_failed ();
      }

      m.remove ("points-to");
      m.set (/*kp + (kp.empty () ? "": "-") + */"points-to", c);
    }

    //
    // Process container.
    //

    void
    process_container_value (semantics::type& t,
                             semantics::data_member& m,
                             string const& prefix,
                             bool obj_ptr)
    {
      if (composite_wrapper (t))
        return;

      if (obj_ptr)
        process_object_pointer (m, t, prefix);
    }

    bool
    process_container (semantics::data_member& m, semantics::type& t)
    {
      // The overall idea is as follows: try to instantiate the container
      // traits class template. If we are successeful, then this is a
      // container type and we can extract the various information from
      // the instantiation. Otherwise, this is not a container.
      //
      location ml (m.location ());

      container_kind_type ck;
      bool smart;
      semantics::type* vt (0);
      semantics::type* it (0);
      semantics::type* kt (0);

      semantics::names* vh (0);
      semantics::names* ih (0);
      semantics::names* kh (0);

      if (t.count ("container-kind"))
      {
        ck = t.get<container_kind_type> ("container-kind");
        smart = t.get<bool> ("container-smart");

        vt = &utype (m, vh, "value");

        if (ck == ck_ordered)
          it = &utype (m, ih, "index");

        if (ck == ck_map || ck == ck_multimap)
          kt = &utype (m, kh, "key");
      }
      else
      {
        tree inst (instantiate_template (container_traits_, t.tree_node ()));

        if (inst == 0)
          return false;

        // @@ This points to the primary template, not the specialization.
        //
        tree decl (TYPE_NAME (inst));

        string f (DECL_SOURCE_FILE (decl));
        size_t l (DECL_SOURCE_LINE (decl));
        size_t c (DECL_SOURCE_COLUMN (decl));

        // Determine the container kind.
        //
        try
        {
          tree decl (
            lookup_qualified_name (
              inst, get_identifier ("kind"), false, false));

          if (decl == error_mark_node || TREE_CODE (decl) != VAR_DECL)
            throw operation_failed ();

          // Instantiate this decalaration so that we can get its value.
          //
          if (DECL_TEMPLATE_INSTANTIATION (decl) &&
              !DECL_TEMPLATE_INSTANTIATED (decl) &&
              !DECL_EXPLICIT_INSTANTIATION (decl))
            instantiate_decl (decl, false, false);

          tree init (DECL_INITIAL (decl));

          if (init == error_mark_node || TREE_CODE (init) != INTEGER_CST)
            throw operation_failed ();

          ck = static_cast<container_kind_type> (integer_value (init));
        }
        catch (operation_failed const&)
        {
          os << f << ":" << l << ":" << c << ": error: "
             << "container_traits specialization does not define the "
             << "container kind constant" << endl;

          throw;
        }

        t.set ("container-kind", ck);

        // See if it is a smart container.
        //
        try
        {
          tree decl (
            lookup_qualified_name (
              inst, get_identifier ("smart"), false, false));

          if (decl == error_mark_node || TREE_CODE (decl) != VAR_DECL)
            throw operation_failed ();

          // Instantiate this decalaration so that we can get its value.
          //
          if (DECL_TEMPLATE_INSTANTIATION (decl) &&
              !DECL_TEMPLATE_INSTANTIATED (decl) &&
              !DECL_EXPLICIT_INSTANTIATION (decl))
            instantiate_decl (decl, false, false);

          tree init (DECL_INITIAL (decl));

          if (init == error_mark_node || TREE_CODE (init) != INTEGER_CST)
            throw operation_failed ();

          smart = static_cast<bool> (integer_value (init));
        }
        catch (operation_failed const&)
        {
          os << f << ":" << l << ":" << c << ": error: "
             << "container_traits specialization does not define the "
             << "'smart' constant" << endl;
          throw;
        }

        // For now we only support ordered smart containers.
        //
        if (smart && ck != ck_ordered)
        {
          os << f << ":" << l << ":" << c << ": error: only ordered smart " <<
            "containers are currently supported" << endl;
          throw operation_failed ();
        }

        t.set ("container-smart", smart);

        // Mark id column as not null.
        //
        t.set ("id-not-null", true);

        // Get the value type.
        //
        {
          tree decl (lookup_qualified_name (
                       inst, get_identifier ("value_type"), true, false));

          if (decl == error_mark_node || TREE_CODE (decl) != TYPE_DECL)
          {
            os << f << ":" << l << ":" << c << ": error: "
               << "container_traits specialization does not define the "
               << "value_type type" << endl;

            throw operation_failed ();
          }

          tree type (TYPE_MAIN_VARIANT (TREE_TYPE (decl)));

          if (semantics::node* n = unit.find (type))
            vt = &dynamic_cast<semantics::type&> (*n);
          else
          {
            error (ml) << "container value type is not instantiated" << endl;
            info (ml) << "use typedef/using to instantiate" << endl;
            throw operation_failed ();
          }

          // Find the hint.
          //
          vh = find_hint (unit, decl);
        }


        t.set ("value-tree-type", vt);
        t.set ("value-tree-hint", vh);
        vt = &utype (m, vh, "value"); // Map.

        // Issue a warning if we are relaxing null-ness in the container
        // type.
        //
        if (t.count ("value-null") && vt->count ("not-null"))
        {
          os << t.file () << ":" << t.line () << ":" << t.column () << ":"
             << " warning: container value declared null while its type "
             << "is declared not null" << endl;
        }

        // Get the index type for ordered containers.
        //
        if (ck == ck_ordered)
        {
          tree decl (
            lookup_qualified_name (
              inst, get_identifier ("index_type"), true, false));

          if (decl == error_mark_node || TREE_CODE (decl) != TYPE_DECL)
          {
            os << f << ":" << l << ":" << c << ": error: "
               << "container_traits specialization does not define the "
               << "index_type type" << endl;
            throw operation_failed ();
          }

          tree type (TYPE_MAIN_VARIANT (TREE_TYPE (decl)));

          if (semantics::node* n = unit.find (type))
            it = &dynamic_cast<semantics::type&> (*n);
          else
          {
            error (ml) << "container index type is not instantiated" << endl;
            info (ml) << "use typedef/using to instantiate" << endl;
            throw operation_failed ();
          }

          // Find the hint.
          //
          ih = find_hint (unit, decl);

          t.set ("index-not-null", true);
          t.set ("index-tree-type", it);
          t.set ("index-tree-hint", ih);
          it = &utype (m, ih, "index"); // Map.
        }

        // Get the key type for maps.
        //
        if (ck == ck_map || ck == ck_multimap)
        {
          tree decl (
            lookup_qualified_name (
              inst, get_identifier ("key_type"), true, false));

          if (decl == error_mark_node || TREE_CODE (decl) != TYPE_DECL)
          {
            os << f << ":" << l << ":" << c << ": error: "
               << "container_traits specialization does not define the "
               << "key_type type" << endl;
            throw operation_failed ();
          }

          tree type (TYPE_MAIN_VARIANT (TREE_TYPE (decl)));

          if (semantics::node* n = unit.find (type))
            kt = &dynamic_cast<semantics::type&> (*n);
          else
          {
            error (ml) << "container key type is not instantiated" << endl;
            info (ml) << "use typedef/using to instantiate" << endl;
            throw operation_failed ();
          }

          // Find the hint.
          //
          kh = find_hint (unit, decl);

          t.set ("key-tree-type", kt);
          t.set ("key-tree-hint", kh);
          kt = &utype (m, kh, "key"); // Map.

          // Issue a warning if we are relaxing null-ness in the container
          // type.
          //
          if (t.count ("key-null") && kt->count ("not-null"))
          {
            os << t.file () << ":" << t.line () << ":" << t.column () << ":"
               << " warning: container key declared null while its type "
               << "is declared not null" << endl;
          }
        }

        // Determine if container value/index/key types are wrappers.
        //
        process_wrapper (*vt);

        if (it != 0)
          process_wrapper (*it);

        if (kt != 0)
          process_wrapper (*kt);

        // Check if we are versioned. For now we are not allowing for
        // soft-add/delete in container keys (might be used in WHERE,
        // primary key).
        //
        {
          semantics::class_* comp (0);
          switch (ck)
          {
          case ck_ordered:
            {
              comp = composite_wrapper (*vt);
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              comp = composite_wrapper (*kt);
              if (comp == 0 || column_count (*comp).soft == 0)
              {
                comp = composite_wrapper (*vt);
                break;
              }

              error (ml) << "map key type cannot have soft-added/deleted " <<
                "data members" << endl;
              info (kt->location ()) << "key type is defined here" << endl;
              throw operation_failed ();
            }
          case ck_set:
          case ck_multiset:
            {
              comp = composite_wrapper (*vt);
              if (comp == 0 || column_count (*comp).soft == 0)
              {
                comp = 0;
                break;
              }

              error (ml) << "set value type cannot have soft-added/deleted " <<
                "data members" << endl;
              info (vt->location ()) << "value type is defined here" << endl;
              throw operation_failed ();
            }
          }

          if (force_versioned || (comp != 0 && column_count (*comp).soft != 0))
            t.set ("versioned", true);
        }
      }

      // Process member data.
      //
      m.set ("id-tree-type", &id_tree_type);

      // Has to be first to handle inverse.
      //
      process_container_value (*vt, m, "value", true);

      if (it != 0)
        process_container_value (*it, m, "index", false);

      if (kt != 0)
        process_container_value (*kt, m, "key", true);

      // A map cannot be an inverse container.
      //
      if (m.count ("value-inverse") && (ck == ck_map || ck == ck_multimap))
      {
        os << m.file () << ":" << m.line () << ":" << m.column () << ":"
           << " error: inverse container cannot be a map" << endl;
        throw operation_failed ();
      }

      // If this is an inverse side of a bidirectional object relationship
      // and it is an ordered container, mark it as unordred since there is
      // no concept of order in this construct.
      //
      if (ck == ck_ordered && m.count ("value-inverse"))
        m.set ("unordered", true);

      // Issue an error if we have a non-inverse smart unordered container.
      //
      if (smart && ck == ck_ordered && unordered (m) &&
          !m.count ("value-inverse"))
      {
        error (ml) << "smart ordered container cannot be unordered" << endl;
        throw operation_failed ();
      }

      // Issue a warning if we are relaxing null-ness in the member.
      //
      if (m.count ("value-null") &&
          (t.count ("value-not-null") || vt->count ("not-null")))
      {
        warn (ml) << "container value declared null while the container "
                  << "type or value type declares it as not null" << endl;
      }

      if (ck == ck_map || ck == ck_multimap)
      {
        if (m.count ("key-null") &&
            (t.count ("key-not-null") || kt->count ("not-null")))
        {
          warn (ml) << "container key declared null while the container "
                    << "type or key type declares it as not null" << endl;
        }
      }

      return true;
    }

    //
    // Implementation details (c-tor, helpers).
    //

    data_member ()
    {
      // Find the odb namespace.
      //
      tree odb = lookup_qualified_name (
        global_namespace, get_identifier ("odb"), false, false);

      if (odb == error_mark_node)
      {
        os << unit.file () << ": error: unable to resolve odb namespace"
           << endl;

        throw operation_failed ();
      }

      // Find wrapper traits.
      //
      wrapper_traits_ = lookup_qualified_name (
        odb, get_identifier ("wrapper_traits"), true, false);

      if (wrapper_traits_ == error_mark_node ||
          !DECL_CLASS_TEMPLATE_P (wrapper_traits_))
      {
        os << unit.file () << ": error: unable to resolve wrapper_traits "
           << "in the odb namespace" << endl;

        throw operation_failed ();
      }

      // Find pointer traits.
      //
      pointer_traits_ = lookup_qualified_name (
        odb, get_identifier ("pointer_traits"), true, false);

      if (pointer_traits_ == error_mark_node ||
          !DECL_CLASS_TEMPLATE_P (pointer_traits_))
      {
        os << unit.file () << ": error: unable to resolve pointer_traits "
           << "in the odb namespace" << endl;

        throw operation_failed ();
      }

      // Find the access class.
      //
      tree access = lookup_qualified_name (
        odb, get_identifier ("access"), true, false);

      if (access == error_mark_node)
      {
        os << unit.file () << ": error: unable to resolve access class"
           << "in the odb namespace" << endl;

        throw operation_failed ();
      }

      access = TREE_TYPE (access);

      // Find container_traits.
      //
      container_traits_ = lookup_qualified_name (
        access, get_identifier ("container_traits"), true, false);

      if (container_traits_ == error_mark_node ||
          !DECL_CLASS_TEMPLATE_P (container_traits_))
      {
        os << unit.file () << ": error: unable to resolve container_traits "
           << "in the odb namespace" << endl;

        throw operation_failed ();
      }
    }

    static tree
    instantiate_template (tree t, tree arg)
    {
      tree args (make_tree_vec (1));
      TREE_VEC_ELT (args, 0) = arg;

      // This step should succeed regardles of whether there is a
      // specialization for this type.
      //
      tree inst (
        lookup_template_class (t, args, 0, 0, 0, tf_warning_or_error));

      if (inst == error_mark_node)
      {
        // Diagnostics has already been issued by lookup_template_class.
        //
        throw operation_failed ();
      }

      inst = TYPE_MAIN_VARIANT (inst);

      // The instantiation may already be complete if it matches a
      // (complete) specialization or was used before.
      //
      if (!COMPLETE_TYPE_P (inst))
        inst = instantiate_class_template (inst);

      // If we cannot instantiate this type, assume there is no suitable
      // specialization for it.
      //
      if (inst == error_mark_node || !COMPLETE_TYPE_P (inst))
        return 0;

      return inst;
    }

  private:
    tree wrapper_traits_;
    tree pointer_traits_;
    tree container_traits_;

    cxx_string_lexer lex_;
  };

  struct view_data_member: traversal::data_member, context
  {
    view_data_member (semantics::class_& c)
        : view_ (c),
          amap_ (c.get<view_alias_map> ("alias-map")),
          omap_ (c.get<view_object_map> ("object-map")) {}

    virtual void
    traverse (semantics::data_member& m)
    {
      using semantics::data_member;

      if (transient (m))
        return;

      semantics::type& t (utype (m));

      if (semantics::class_* c = object_pointer (t))
      {
        location const& l (m.location ());

        if (lazy_pointer (t))
        {
          error (l) << "lazy object pointer in view" << endl;
          throw operation_failed ();
        }

        // Find the corresponding associated object. First see if this
        // data member name matches any aliases.
        //
        view_alias_map::iterator i (amap_.find (m.name ()));

        if (i == amap_.end ())
          i = amap_.find (public_name (m, false));

        view_object* vo (0);

        if (i != amap_.end ())
        {
          vo = i->second;

          if (vo->obj != c) // @@ Poly base/derived difference?
          {
            error (l) << "different pointed-to and associated objects" << endl;
            info (vo->loc) << "associated object is defined here" << endl;
            throw operation_failed ();
          }
        }
        else
        {
          // If there is no alias match, try the object type.
          //
          view_object_map::iterator i (omap_.find (c));

          if (i == omap_.end ())
          {
            error (l) << "unable to find associated object for object "
                      << "pointer" << endl;
            info (l) << "use associated object alias as this data member "
                     << "name" << endl;
            throw operation_failed ();
          }

          vo = i->second;
        }

        if (vo->ptr != 0)
        {
          location const& l2 (vo->ptr->location ());

          error (l) << "associated object is already loaded via another "
                    << "object pointer" << endl;
          info (l2) << "the other data member is defined here" << endl;
          info (l2) << "use associated object alias as this data member "
                    << "name to load a different object" << endl;

          throw operation_failed ();
        }

        vo->ptr = &m;
        m.set ("view-object", vo);
      }
    }

  private:
    semantics::class_& view_;
    view_alias_map& amap_;
    view_object_map& omap_;
  };

  // Figure out the "summary" added/deleted version for a composite
  // value type.
  //
  struct summary_version: object_members_base
  {
    summary_version (): av (0), dv (0), a_ (true), d_ (true) {}

    virtual void
    traverse_simple (semantics::data_member&)
    {
      if (a_)
      {
        if (unsigned long long v = added (member_path_))
        {
          if (av == 0 || av < v)
            av = v;
        }
        else
        {
          av = 0;
          a_ = false;
        }
      }

      if (d_)
      {
        if (unsigned long long v = deleted (member_path_))
        {
          if (dv == 0 || dv > v)
            dv = v;
        }
        else
        {
          dv = 0;
          d_ = false;
        }
      }
    }

  public:
    unsigned long long av;
    unsigned long long dv;

    bool a_;
    bool d_;
  };

  struct class_: traversal::class_, context
  {
    class_ ()
        : typedefs_ (true),
          std_string_ (0),
          std_string_hint_ (0),
          access_ (0)
    {
      *this >> defines_ >> *this;
      *this >> typedefs_ >> *this;

      member_names_ >> member_;

      // Resolve the std::string type node.
      //
      using semantics::scope;

      for (scope::names_iterator_pair ip (unit.find ("std"));
           ip.first != ip.second; ++ip.first)
      {
        if (scope* ns = dynamic_cast<scope*> (&ip.first->named ()))
        {
          scope::names_iterator_pair jp (ns->find ("string"));

          if (jp.first != jp.second)
          {
            std_string_ = dynamic_cast<semantics::type*> (
              &jp.first->named ());
            std_string_hint_ = &*jp.first;
            break;
          }
        }
      }

      assert (std_string_ != 0); // No std::string?

      // Resolve odb::access, if any.
      //
      tree odb = lookup_qualified_name (
        global_namespace, get_identifier ("odb"), false, false);

      if (odb != error_mark_node)
      {
        access_ = lookup_qualified_name (
          odb, get_identifier ("access"), true, false);

        access_ = (access_ != error_mark_node ? TREE_TYPE (access_) : 0);
      }
    }

    virtual void
    traverse (type& c)
    {
      class_kind_type k (class_kind (c));

      if (k == class_other)
        return;

      names (c); // Process nested classes.

      // Check if odb::access is a friend of this class.
      //
      c.set ("friend", access_ != 0 && is_friend (c.tree_node (), access_));

      // Assign pointer.
      //
      if (k == class_object || k == class_view)
        assign_pointer (c);

      if (k == class_object)
        traverse_object_pre (c);
      else if (k == class_view)
        traverse_view_pre (c);

      names (c, member_names_);

      if (k == class_object)
        traverse_object_post (c);
      else if (k == class_view)
        traverse_view_post (c);
      else if (k == class_composite)
        traverse_composite_post (c);
    }

    //
    // Object.
    //

    virtual void
    traverse_object_pre (type& c)
    {
      using semantics::class_;
      using semantics::data_member;

      class_* poly_root (polymorphic (c));

      // Sections.
      //
      user_sections& uss (c.set ("user-sections", user_sections (c)));

      // Copy sections from reuse bases. For polymorphic classes, sections
      // are overridden.
      //
      if (poly_root == 0 || poly_root == &c)
      {
        for (type::inherits_iterator i (c.inherits_begin ());
             i != c.inherits_end (); ++i)
        {
          type& b (i->base ());

          if (object (b))
          {
            user_sections& buss (b.get<user_sections> ("user-sections"));

            for (user_sections::iterator j (buss.begin ());
                 j != buss.end ();
                 ++j)
            {
              // Don't copy the special version update section.
              //
              if (j->special == user_section::special_version)
                continue;

              uss.push_back (*j);
              uss.back ().object = &c;
              uss.back ().base = &*j;
            }
          }
        }
      }

      // Determine whether it is a session object.
      //
      if (!c.count ("session"))
      {
        // If this is a derived class in a polymorphic hierarchy,
        // then it should have the same session value as the root.
        //
        if (poly_root != 0 && poly_root != &c)
          c.set ("session", session (*poly_root));
        else
        {
          // See if any of the namespaces containing this class specify
          // the session value.
          //
          bool found (false);
          for (semantics::scope* s (&class_scope (c));; s = &s->scope_ ())
          {
            using semantics::namespace_;

            namespace_* ns (dynamic_cast<namespace_*> (s));

            if (ns == 0) // Some other scope.
            {
              if (!s->named_p ())
                break;

              continue;
            }

            if (ns->extension ())
              ns = &ns->original ();

            if (ns->count ("session"))
            {
              c.set ("session", ns->get<bool> ("session"));
              found = true;
              break;
            }

            if (ns->global_scope ()) // Note: namespaces always named.
              break;
          }

          // If still not found, then use the default value.
          //
          if (!found)
            c.set ("session", options.generate_session ());
        }
      }

      if (session (c))
        features.session_object = true;

      if (poly_root != 0)
      {
        using namespace semantics;
        using semantics::data_member;

        data_member_path& id (*id_member (*poly_root));
        data_member* idm (id.front ());

        if (poly_root != &c)
        {
          // If we are a derived class in the polymorphic persistent
          // class hierarchy, then add a synthesized virtual pointer
          // member that points back to the root.
          //
          semantics::class_& base (polymorphic_base (c));

          if (&base != poly_root)
            idm = &dynamic_cast<data_member&> (base.names_begin ()->named ());

          path const& f (idm->file ());
          size_t l (idm->line ()), col (idm->column ());

          semantics::data_member& m (
            unit.new_node<semantics::data_member> (f, l, col, tree (0)));
          m.set ("virtual", true);

          // Make it the first member in the class. This is important:
          // we rely on the corrensponding foreign key to be first.
          //
          node_position<type, scope::names_iterator> np (c, c.names_end ());
          unit.new_edge<semantics::names> (
            np, m, idm->name (), access::public_);

          // Use the raw pointer as this member's type.
          //
          if (!base.pointed_p ())
          {
            // Create the pointer type in the graph. The pointer node
            // in GCC seems to always be present, even if not explicitly
            // used in the translation unit.
            //
            tree t (base.tree_node ());
            tree ptr (TYPE_POINTER_TO (t));
            assert (ptr != 0);
            ptr = TYPE_MAIN_VARIANT (ptr);
            pointer& p (unit.new_node<pointer> (f, l, col, ptr));
            unit.insert (ptr, p);
            unit.new_edge<points> (p, base);
            assert (base.pointed_p ());
          }

          unit.new_edge<belongs> (m, base.pointed ().pointer ());

          // Mark it as a special kind of id.
          //
          m.set ("id", string ());
          m.set ("polymorphic-ref", true);

          // Make sure we also use the same column name as the root.
          //
          if (composite_wrapper (utype (id)))
            m.set ("column", table_column (column_prefix (id, true).prefix));
          else
            m.set ("column", table_column (column_name (id)));
        }
        else
        {
          // If we are a root of the polymorphic persistent class hierarchy,
          // then add a synthesized virtual member for the discriminator.
          // Use the location of the polymorphic pragma as the location of
          // this member.
          //
          location_t loc (c.get<location_t> ("polymorphic-location"));
          semantics::data_member& m (
            unit.new_node<semantics::data_member> (
              path (LOCATION_FILE (loc)),
              LOCATION_LINE (loc),
              LOCATION_COLUMN (loc),
              tree (0)));
          m.set ("virtual", true);

          // Insert it after the id member (or first if this id comes
          // from reuse-base).
          //
          node_position<type, scope::names_iterator> np (
            c, c.find (idm->named ()));
          unit.new_edge<semantics::names> (
            np, m, "typeid_", access::public_);

          belongs& edge (unit.new_edge<belongs> (m, *std_string_));
          edge.hint (*std_string_hint_);

          m.set ("readonly", true);
          m.set ("discriminator", true);

          c.set ("discriminator", &m);
        }
      }
    }

    virtual void
    traverse_object_post (type& c)
    {
      semantics::class_* poly_root (polymorphic (c));
      bool poly_derived (poly_root != 0 && poly_root != &c);

      semantics::data_member* opt (optimistic (c));

      // Figure out if we are versioned. We are versioned if we have
      // soft-added/deleted columns ourselves or our poly-base is
      // versioned.
      //
      if (force_versioned ||
          column_count (c).soft != 0 ||
          (poly_derived && polymorphic_base (c).count ("versioned")))
        c.set ("versioned", true);

      // Sections.
      //
      user_sections& uss (c.get<user_sections> ("user-sections"));

      // See if we need to add a special fake section for version update.
      //
      if (c.count ("sectionable"))
      {
        uss.push_back (
          user_section (*opt,
                        c,
                        uss.count (user_sections::count_total |
                                   user_sections::count_all   |
                                   user_sections::count_special_version),
                        user_section::load_lazy,
                        user_section::update_manual,
                        user_section::special_version));

        // If we are a root of a polymorphic hierarchy and the version is in
        // a reuse-base, then we need to make sure that base is sectionable
        // and derive from its special version update section.
        //
        semantics::node& opt_base (opt->scope ());
        if (poly_root == &c && &opt_base != &c)
        {
          if (!opt_base.count ("sectionable"))
          {
            location_t l (c.get<location_t> ("sectionable-location"));

            error (l) << "reuse base class of a sectionable polymorphic " <<
              "root class must be sectionable" << endl;

            info (opt_base.location ()) << "use '#pragma db object " <<
              "sectionable' to make the base class of this hierarchy " <<
              "sectionable" << endl;

            throw operation_failed ();
          }

          uss.back ().base =
            &opt_base.get<user_sections> ("user-sections").back ();
        }
      }

      // Calculate column counts for sections.
      //
      for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
      {
        column_count_type cc (column_count (c, &*i));
        i->total = cc.total;
        i->inverse = cc.inverse;
        i->readonly = cc.readonly;

        // Figure out if we are versioned. We are versioned if we have
        // soft-added/deleted columns ourselves or our poly-base is
        // versioned.
        //
        if (force_versioned || cc.soft != 0 ||
            (poly_derived && i->base != 0 && i->base->versioned))
          i->versioned = true;

        if (size_t n = has_a (c, test_container, &*i))
        {
          i->containers = true;
          i->versioned_containers =
            n != has_a (c,
                        test_container |
                        exclude_deleted | exclude_added | exclude_versioned,
                        &*i);

          if ((n = has_a (c, test_readwrite_container, &*i)))
          {
            i->readwrite_containers = true;
            i->readwrite_versioned_containers =
              n != has_a (c,
                          test_readwrite_container |
                          exclude_deleted | exclude_added | exclude_versioned,
                          &*i);
          }
        }
      }
    }

    //
    // View.
    //

    virtual void
    traverse_view_pre (type& c)
    {
      // Resolve referenced objects from tree nodes to semantic graph
      // nodes. Also populate maps and compute counts.
      //
      view_alias_map& amap (c.set ("alias-map", view_alias_map ()));
      view_object_map& omap (c.set ("object-map", view_object_map ()));

      size_t& obj_count (c.set ("object-count", size_t (0)));
      size_t& tbl_count (c.set ("table-count", size_t (0)));

      if (c.count ("objects"))
      {
        using semantics::class_;

        view_objects& objs (c.get<view_objects> ("objects"));

        for (view_objects::iterator i (objs.begin ()); i != objs.end (); ++i)
        {
          if (i->kind != view_object::object)
          {
            tbl_count++;
            continue;
          }
          else
            obj_count++;

          tree n (TYPE_MAIN_VARIANT (i->obj_node));

          if (TREE_CODE (n) != RECORD_TYPE)
          {
            error (i->loc) << "name '" << i->obj_name << "' in db pragma " <<
              "object does not name a class" << endl;

            throw operation_failed ();
          }

          class_& o (dynamic_cast<class_&> (*unit.find (n)));

          if (!object (o))
          {
            error (i->loc) << "name '" << i->obj_name << "' in db pragma " <<
              "object does not name a persistent class" << endl;

            info (o.location ()) << "class '" << i->obj_name << "' is " <<
              "defined here" << endl;

            throw operation_failed ();
          }

          i->obj = &o;
          i->ptr = 0; // Nothing yet.

          if (i->alias.empty ())
          {
            if (!omap.insert (view_object_map::value_type (&o, &*i)).second)
            {
              error (i->loc) << "persistent class '" << i->obj_name <<
                "' is used in the view more than once" << endl;

              error (omap[&o]->loc) << "previously used here" << endl;

              info (i->loc) << "use the alias clause to assign it a " <<
                "different name" << endl;

              throw operation_failed ();
            }

            // Also add the bases of a polymorphic object.
            //
            class_* poly_root (polymorphic (o));

            if (poly_root != 0 && poly_root != &o)
            {
              for (class_* b (&polymorphic_base (o));;
                   b = &polymorphic_base (*b))
              {
                if (!omap.insert (view_object_map::value_type (b, &*i)).second)
                {
                  error (i->loc) << "base class '" << class_name (*b) <<
                    "' is used in the view more than once" << endl;

                  error (omap[b]->loc) << "previously used here" << endl;

                  info (i->loc) << "use the alias clause to assign it a " <<
                    "different name" << endl;

                  throw operation_failed ();
                }

                if (b == poly_root)
                  break;
              }
            }
          }
          else
          {
            if (!amap.insert (
                  view_alias_map::value_type (i->alias, &*i)).second)
            {
              error (i->loc) << "alias '" << i->alias << "' is used in " <<
                "the view more than once" << endl;

              throw operation_failed ();
            }
          }
        }
      }
    }

    virtual void
    traverse_view_post (type& c)
    {
      // Handle data members.
      //
      {
        view_data_member t (c);
        traversal::names n (t);
        names (c, n);
      }

      // Figure out if we are versioned. Forced versioning is handled
      // in relational/processing.
      //
      if (column_count (c).soft != 0)
        c.set ("versioned", true);
    }

    //
    // Composite.
    //

    virtual void
    traverse_composite_post (type& c)
    {
      // Figure out if we are versioned.
      //
      if (force_versioned || column_count (c).soft != 0)
      {
        c.set ("versioned", true);

        // See if we are "summarily" added/deleted, that is, all the
        // columns are added/deleted. Note: this does not include
        // containers.
        //
        summary_version sv;
        sv.traverse (c);

        // Note: there are no locations.
        //
        if (sv.av != 0)
          c.set ("added", sv.av);

        if (sv.dv != 0)
          c.set ("deleted", sv.dv);
      }
    }

    //
    // Assign object/view pointer.
    //

    void
    assign_pointer (type& c)
    {
      location_t loc (0);     // Pragma location, or 0 if not used.

      try
      {
        bool raw;
        string ptr;
        string const& type (class_fq_name (c));

        tree decl (0);          // Resolved template node.
        string decl_name;       // User-provided template name.
        tree resolve_scope (0); // Scope in which we resolve names.

        class_pointer const* cp (0);
        bool cp_template (false);

        if (c.count ("pointer"))
        {
          cp = &c.get<class_pointer> ("pointer");
        }
        // If we are a derived type in polymorphic hierarchy, then use
        // our root's pointer type by default.
        //
        else if (semantics::class_* r = polymorphic (c))
        {
          if (&c != r && r->count ("pointer-template"))
            cp = r->get<class_pointer const*> ("pointer-template");
        }

        if (cp != 0)
        {
          string const& p (cp->name);

          if (p == "*")
          {
            raw = true;
            ptr = type + "*";
            cp_template = true;
          }
          else if (p[p.size () - 1] == '*')
          {
            raw = true;
            ptr = p;
          }
          else if (p.find ('<') != string::npos)
          {
            // Template-id.
            //
            raw = false; // Fair to assume not raw, though technically can be.
            ptr = p;
            decl_name.assign (p, 0, p.find ('<'));
          }
          else
          {
            // This is not a template-id. Resolve it and see if it is a
            // template or a type.
            //
            decl = resolve_name (p, cp->scope, true);
            gcc_tree_code_type tc (TREE_CODE (decl));

            if (tc == TYPE_DECL)
            {
              raw = (TREE_CODE (TREE_TYPE (decl)) == POINTER_TYPE);
              ptr = p;

              // This can be a typedef'ed alias for a TR1 template-id.
              //
              if (tree ti = TYPE_TEMPLATE_INFO (TREE_TYPE (decl)))
              {
                decl = TI_TEMPLATE (ti); // DECL_TEMPLATE

                // Get to the most general template declaration.
                //
                while (DECL_TEMPLATE_INFO (decl))
                  decl = DECL_TI_TEMPLATE (decl);
              }
              else
                decl = 0; // Not a template.
            }
            else if (tc == TEMPLATE_DECL && DECL_CLASS_TEMPLATE_P (decl))
            {
              raw = false;
              ptr = p + "< " + type + " >";
              decl_name = p;
              cp_template = true;
            }
            else
            {
              error (cp->loc)
                << "name '" << p << "' specified with db pragma pointer "
                << "does not name a type or a template" << endl;

              throw operation_failed ();
            }
          }

          // Resolve scope is the scope of the pragma.
          //
          resolve_scope = cp->scope;
          loc = cp->loc;
        }
        else
        {
          // See if any of the namespaces containing this class specify
          // a pointer.
          //
          for (semantics::scope* s (&class_scope (c));; s = &s->scope_ ())
          {
            using semantics::namespace_;

            namespace_* ns (dynamic_cast<namespace_*> (s));

            if (ns == 0) // Some other scope.
            {
              if (!s->named_p ())
                break;

              continue;
            }

            if (ns->extension ())
              ns = &ns->original ();

            if (!ns->count ("pointer"))
            {
              if (ns->global_scope ()) // Note: namespace always named.
                break;
              else
                continue;
            }

            cp = &ns->get<class_pointer> ("pointer");
            string const& p (cp->name);

            // Namespace-specified pointer can only be '*' or are template.
            //
            if (p == "*")
            {
              raw = true;
              ptr = type + "*";
            }
            else if (p[p.size () - 1] == '*')
            {
              error (cp->loc)
                << "name '" << p << "' specified with db pragma pointer "
                << "at namespace level cannot be a raw pointer" << endl;
            }
            else if (p.find ('<') != string::npos)
            {
              error (cp->loc)
                << "name '" << p << "' specified with db pragma pointer "
                << "at namespace level cannot be a template-id" << endl;
            }
            else
            {
              // Resolve this name and make sure it is a template.
              //
              decl = resolve_name (p, cp->scope, true);
              gcc_tree_code_type tc (TREE_CODE (decl));

              if (tc == TEMPLATE_DECL && DECL_CLASS_TEMPLATE_P (decl))
              {
                raw = false;
                ptr = p + "< " + type + " >";
                decl_name = p;
              }
              else
              {
                error (cp->loc)
                  << "name '" << p << "' specified with db pragma pointer "
                  << "does not name a template" << endl;
              }
            }

            if (ptr.empty ())
              throw operation_failed ();

            cp_template = true;

            // Resolve scope is the scope of the pragma.
            //
            resolve_scope = cp->scope;
            loc = cp->loc;
            break;
          }

          // Use the default pointer.
          //
          if (ptr.empty ())
          {
            string const& p (options.default_pointer ());

            if (p == "*")
            {
              raw = true;
              ptr = type + "*";
            }
            else
            {
              raw = false;
              ptr = p + "< " + type + " >";
              decl_name = p;
            }

            // Resolve scope is the scope of the class.
            //
            resolve_scope = class_scope (c).tree_node ();
          }
        }

        // If this class is a root of a polymorphic hierarchy, then cache
        // the pointer template so that we can use it for derived classes.
        //
        if (cp != 0 && cp_template && polymorphic (c) == &c)
          c.set ("pointer-template", cp);

        // Check if we are using TR1.
        //
        if (decl != 0 || !decl_name.empty ())
        {
          bool& tr1 (features.tr1_pointer);
          bool& boost (features.boost_pointer);

          // First check the user-supplied name.
          //
          tr1 = tr1
            || decl_name.compare (0, 8, "std::tr1") == 0
            || decl_name.compare (0, 10, "::std::tr1") == 0;

          // If there was no match, also resolve the name since it can be
          // a using-declaration for a TR1 template.
          //
          if (!tr1)
          {
            if (decl == 0)
              decl = resolve_name (decl_name, resolve_scope, false);

            if (TREE_CODE (decl) != TEMPLATE_DECL || !
                DECL_CLASS_TEMPLATE_P (decl))
            {
              // This is only checked for the --default-pointer option.
              //
              error (c.file (), c.line (), c.column ())
                << "name '" << decl_name << "' specified with the "
                << "--default-pointer option does not name a class "
                << "template" << endl;

              throw operation_failed ();
            }

            string n (decl_as_string (decl, TFF_PLAIN_IDENTIFIER));

            // In case of a boost TR1 implementation, we cannot distinguish
            // between the boost:: and std::tr1:: usage since the latter is
            // just a using-declaration for the former.
            //
            tr1 = tr1
              || n.compare (0, 8, "std::tr1") == 0
              || n.compare (0, 10, "::std::tr1") == 0;

            boost = boost
              || n.compare (0, 17, "boost::shared_ptr") == 0
              || n.compare (0, 19, "::boost::shared_ptr") == 0;
          }
        }

        // Fully-qualify all the unqualified components of the name.
        //
        try
        {
          lex_.start (ptr);
          ptr.clear ();

          string t;
          bool punc (false);
          bool scoped (false);

          for (cpp_ttype tt (lex_.next (t));
               tt != CPP_EOF;
               tt = lex_.next (t))
          {
            if (punc && tt > CPP_LAST_PUNCTUATOR)
              ptr += ' ';

            punc = false;

            switch (static_cast<unsigned> (tt))
            {
            case CPP_LESS:
              {
                ptr += "< ";
                break;
              }
            case CPP_GREATER:
              {
                ptr += " >";
                break;
              }
            case CPP_COMMA:
              {
                ptr += ", ";
                break;
              }
            case CPP_NAME:
              {
                // If the name was not preceeded with '::', look it
                // up in the pragmas's scope and add the qualifer.
                //
                if (!scoped)
                {
                  tree decl (resolve_name (t, resolve_scope, false));
                  tree scope (CP_DECL_CONTEXT (decl));

                  // If this is an inline namespace, skip it until we get
                  // to the non-inline one.
                  //
                  while (scope != global_namespace)
                  {
                    tree prev (CP_DECL_CONTEXT (scope));

#if BUILDING_GCC_MAJOR >= 8
                    if (!is_nested_namespace (prev, scope, true))
#else
                    if (!is_associated_namespace (prev, scope))
#endif
                      break;

                    scope = prev;
                  }

                  if (scope != global_namespace)
                  {
                    ptr += "::";
                    ptr += decl_as_string (scope, TFF_PLAIN_IDENTIFIER);
                  }

                  ptr += "::";
                }

                ptr += t;
                punc = true;
                break;
              }
            case CPP_KEYWORD:
            case CPP_NUMBER:
              {
                ptr += t;
                punc = true;
                break;
              }
            default:
              {
                ptr += t;
                break;
              }
            }

            scoped = (tt == CPP_SCOPE);
          }
        }
        catch (cxx_lexer::invalid_input const&)
        {
          throw operation_failed ();
        }

        c.set ("object-pointer", ptr);
        c.set ("object-pointer-raw", raw);
      }
      catch (invalid_name const& ex)
      {
        if (loc != 0)
          error (loc)
            << "name '" << ex.name () << "' specified with db pragma "
            << "pointer is invalid" << endl;
        else
          error (c.file (), c.line (), c.column ())
            << "name '" << ex.name () << "' specified with the "
            << "--default-pointer option is invalid" << endl;


        throw operation_failed ();
      }
      catch (unable_to_resolve const& ex)
      {
        if (loc != 0)
          error (loc)
            << "unable to resolve name '" << ex.name () << "' specified "
            << "with db pragma pointer" << endl;
        else
          error (c.file (), c.line (), c.column ())
            << "unable to resolve name '" << ex.name () << "' specified "
            << "with the --default-pointer option" << endl;

        throw operation_failed ();
      }
    }

  private:
    struct invalid_name
    {
      invalid_name (string const& n): name_ (n) {}

      string const&
      name () const {return name_;}

    private:
      string name_;
    };

    typedef lookup::unable_to_resolve unable_to_resolve;

    tree
    resolve_name (string const& qn, tree scope, bool is_type)
    {
      try
      {
        string tl;
        tree tn;
        cpp_ttype tt, ptt;

        nlex_.start (qn);
        tt = nlex_.next (tl, &tn);

        string name;
        return lookup::resolve_scoped_name (
          nlex_, tt, tl, tn, ptt, scope, name, is_type);
      }
      catch (cxx_lexer::invalid_input const&)
      {
        throw invalid_name (qn);
      }
      catch (lookup::invalid_name const&)
      {
        throw invalid_name (qn);
      }
    }

  private:
    traversal::defines defines_;
    typedefs typedefs_;

    data_member member_;
    traversal::names member_names_;

    cxx_string_lexer lex_;
    cxx_string_lexer nlex_; // Nested lexer.

    semantics::type* std_string_;
    semantics::names* std_string_hint_;

    tree access_; // odb::access node.
  };

  static bool
  check_to_from (const cxx_tokens& ex, const char* c, location_t l)
  {
    // Make sure we have one and only one placeholder (?).
    //
    bool r (false), m (true);

    for (cxx_tokens::const_iterator i (ex.begin ()), e (ex.end ()); i != e;)
    {
      if (i->type == CPP_OPEN_PAREN)
      {
        if (++i != e && i->type == CPP_QUERY)
        {
          if (++i != e && i->type == CPP_CLOSE_PAREN)
          {
            if (r)
              m = false; // Multiple (?), can't move.
            else
              r = true;
          }
        }
      }
      else
        ++i;
    }

    if (!r)
    {
      error (l) << "no '(?)' expression in the '" << c << "' clause "
                << "of db pragma map" << endl;

      throw operation_failed ();
    }

    return m;
  }
}

static void
process1 (semantics::unit& u)
{
  // Process custom C++ type mapping.
  //

  // Create an empty list if we don't have one. This makes the
  // rest of the code simpler.
  //
  if (!u.count ("custom-cxx-types"))
    u.set ("custom-cxx-types", custom_cxx_types ());

  custom_cxx_types & cts (u.get<custom_cxx_types> ("custom-cxx-types"));

  for (custom_cxx_types::iterator i (cts.begin ()); i != cts.end (); ++i)
  {
    custom_cxx_type& ct (*i);

    // type
    //
    if (ct.type_node == 0)
    {
      error (ct.loc) << "'type' clause expected in db pragma map" << endl;
      throw operation_failed ();
    }

    ct.type = dynamic_cast<semantics::type*> (
      u.find (TYPE_MAIN_VARIANT (ct.type_node)));
    ct.type_hint = u.find_hint (ct.type_node);

    // as
    //
    if (ct.as_node == 0)
    {
      error (ct.loc) << "'as' clause expected in db pragma map" << endl;
      throw operation_failed ();
    }

    ct.as = dynamic_cast<semantics::type*> (
      u.find (TYPE_MAIN_VARIANT (ct.as_node)));
    ct.as_hint = u.find_hint (ct.as_node);

    // to
    //
    {
      cxx_tokens& e (ct.to);

      if (e.empty ())
      {
        e.push_back (cxx_token (0, CPP_OPEN_PAREN));
        e.push_back (cxx_token (0, CPP_QUERY));
        e.push_back (cxx_token (0, CPP_CLOSE_PAREN));
        ct.to_move = true;
      }
      else
        ct.to_move = check_to_from (e, "to", ct.loc);
    }

    // from
    //
    {
      cxx_tokens& e (ct.from);

      if (e.empty ())
      {
        e.push_back (cxx_token (0, CPP_OPEN_PAREN));
        e.push_back (cxx_token (0, CPP_QUERY));
        e.push_back (cxx_token (0, CPP_CLOSE_PAREN));
        ct.from_move = true;
      }
      else
        ct.from_move = check_to_from (e, "from", ct.loc);
    }

    // Resolve mapping scope.
    //
    semantics::scope* s (dynamic_cast<semantics::scope*> (u.find (ct.scope)));
    if (s == 0)
    {
      error (ct.loc) << "unable to resolve db pragma map scope" << endl;
      throw operation_failed ();
    }

    if (semantics::namespace_* ns = dynamic_cast<semantics::namespace_*> (s))
    {
      if (ns->extension ())
        s = &ns->original ();
    }

    // Enter into the map.
    //
    if (!s->count ("custom-cxx-type-map"))
      s->set ("custom-cxx-type-map", custom_cxx_type_map ());

    s->get<custom_cxx_type_map> ("custom-cxx-type-map")[ct.type] = &ct;
  }
}

static void
process2 (options const& ops, features& f, semantics::unit& u)
{
  unique_ptr<context> ctx (create_context (cerr, u, ops, f, 0));

  // Common processing.
  //
  {
    traversal::unit unit;
    traversal::defines unit_defines;
    typedefs unit_typedefs (true);
    traversal::namespace_ ns;
    class_ c;

    unit >> unit_defines >> ns;
    unit_defines >> c;
    unit >> unit_typedefs >> c;

    traversal::defines ns_defines;
    typedefs ns_typedefs (true);

    ns >> ns_defines >> ns;
    ns_defines >> c;
    ns >> ns_typedefs >> c;

    unit.dispatch (ctx->unit);
  }

  // Database-specific processing.
  //
  switch (ops.database ()[0])
  {
  case database::common:
    {
      break;
    }
  case database::mssql:
  case database::mysql:
  case database::oracle:
  case database::pgsql:
  case database::sqlite:
    {
      relational::process ();
      break;
    }
  }
}

void
process (options const& ops,
         features& f,
         semantics::unit& u,
         semantics::path const&,
         unsigned short pass)
{
  try
  {
    if (pass == 1)
      process1 (u);
    else if (pass == 2)
      process2 (ops, f, u);
  }
  catch (operation_failed const&)
  {
    // Processing failed. Diagnostics has already been issued.
    //
    throw processor_failed ();
  }
}
