// file      : odb/validator.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <set>
#include <iostream>

#include <odb/traversal.hxx>
#include <odb/common.hxx>
#include <odb/context.hxx>
#include <odb/diagnostics.hxx>
#include <odb/validator.hxx>
#include <odb/cxx-lexer.hxx>

#include <odb/relational/validator.hxx>

using namespace std;

namespace
{
  // Resolve null overrides.
  //
  static void
  override_null (semantics::node& n, string const& prefix = "")
  {
    string p (prefix.empty () ? prefix : prefix + '-');

    if (n.count (p + "null") && n.count (p + "not-null"))
    {
      if (n.get<location_t> (p + "null-location") <
          n.get<location_t> (p + "not-null-location"))
      {
        n.remove (p + "null");
        n.remove (p + "null-location");
      }
      else
      {
        n.remove (p + "not-null");
        n.remove (p + "not-null-location");
      }
    }
  }

  //
  // Pass 1.
  //

  struct data_member1: traversal::data_member, context
  {
    data_member1 (bool& valid)
        : valid_ (valid)
    {
    }

    virtual void
    traverse (type& m)
    {
      semantics::class_& c (dynamic_cast<semantics::class_&> (m.scope ()));
      bool obj (object (c));

      // If the class is marked transient, then mark each non-virtual
      // data member as transient.
      //
      {
        bool t (transient (m));

        if (!t && c.count ("transient") && !m.count ("virtual"))
        {
          m.set ("transient", true);
          t = true;
        }

        if (t)
          return;
      }

      semantics::names* hint;
      semantics::type& t (utype (m, hint));

      if (t.fq_anonymous (hint))
      {
        os << m.file () << ":" << m.line () << ":" << m.column () << ":"
           << " error: unnamed type in data member declaration" << endl;

        os << m.file () << ":" << m.line () << ":" << m.column () << ":"
           << " info: use 'typedef' to name this type" << endl;

        valid_ = false;
      }

      // Make sure id or inverse member is not marked readonly since we
      // depend on these three sets not having overlaps.
      //
      if (m.count ("readonly")) // context::readonly() also checks the class.
      {
        if (id (m))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ":"
             << " error: object id should not be declared readonly" << endl;

          valid_ = false;
        }

        if (inverse (m))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ":"
             << " error: inverse object pointer should not be declared "
             << "readonly" << endl;

          valid_ = false;
        }
      }

      // Make sure a member of a section is an immediate member of an object.
      // The same for the section member itself.
      //
      bool section (t.fq_name () == "::odb::section");

      if (!obj)
      {
        if (m.count ("section-member"))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": " <<
            "error: data member belonging to a section can only be a " <<
            "direct member of a persistent class" << endl;
          valid_ = false;
        }

        if (section)
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": " <<
            "error: section data member can only be a direct member of a " <<
            "persistent class" << endl;
          valid_ = false;
        }
      }

      // Make sure the load and update pragmas are only specified on
      // section members.
      //
      if (!section)
      {
        if (m.count ("section-load"))
        {
          location_t loc (m.get<location_t> ("section-load-location"));
          error (loc) << "'#pragma db load' can only be specified for "
            "a section data member" << endl;
          valid_ = false;
        }

        if (m.count ("section-update"))
        {
          location_t loc (m.get<location_t> ("section-update-location"));
          error (loc) << "'#pragma db update' can only be specified for "
            "a section data member" << endl;
          valid_ = false;
        }
      }

      // Check that the addition version makes sense.
      //
      unsigned long long av (m.get<unsigned long long> ("added", 0));
      if (av != 0)
      {
        location_t l (m.get<location_t> ("added-location"));

        if (id (m))
        {
          error (l) << "object id cannod be soft-added" << endl;
          valid_ = false;
        }

        if (version (m))
        {
          error (l) << "optimistic concurrency version cannod be "
            "soft-added" << endl;
          valid_ = false;
        }

        if (!versioned ())
        {
          error (l) << "added data member in a non-versioned object " <<
            "model" << endl;
          valid_ = false;
        }
        else
        {
          model_version const& mv (version ());

          if (av > mv.current)
          {
            error (l) << "addition version is greater than the current " <<
              "model version" << endl;
            valid_ = false;
          }
          else if (av <= mv.base)
          {
            error (l) << "addition version is less than or equal to the " <<
              "base model version" << endl;
            info (l) << "delete this pragma since migration to version " <<
              av << " is no longer possible" << endl;
            valid_ = false;
          }
        }
      }

      // Check that the deletion version makes sense.
      //
      unsigned long long dv (m.get<unsigned long long> ("deleted", 0));
      if (dv != 0)
      {
        location_t l (m.get<location_t> ("deleted-location"));

        if (id (m))
        {
          error (l) << "object id cannod be soft-deleted" << endl;
          valid_ = false;
        }

        if (version (m))
        {
          error (l) << "optimistic concurrency version cannod be "
            "soft-deleted" << endl;
          valid_ = false;
        }

        if (!versioned ())
        {
          error (l) << "deleted data member in a non-versioned object " <<
            "model" << endl;
          valid_ = false;
        }
        else
        {
          model_version const& mv (version ());

          if (dv > mv.current)
          {
            error (l) << "deletion version is greater than the current " <<
              "model version" << endl;
            valid_ = false;
          }
          else if (dv <= mv.base)
          {
            error (l) << "deletion version is less than or equal to the " <<
              "base model version" << endl;
            info (c.location ()) << "delete this data member since " <<
              "migration to version " << dv << " is no longer possible" <<
              endl;
            valid_ = false;
          }
        }
      }

      // Make sure that addition and deletion versions are properly ordered.
      // We can have both the [av, dv] (added then deleted) and [dv, av]
      // (deleted then re-added) intervals.
      //
      if (av != 0 && dv != 0 && av == dv)
      {
        location_t al (m.get<location_t> ("added-location"));
        location_t dl (m.get<location_t> ("deleted-location"));

        error (al) << "addition and deletion versions are the same" << endl;
        info (dl) << "deletion version is specified here" << endl;
        valid_ = false;
      }

      if (section)
        return; // Section data member is transient.

      // Resolve null overrides.
      //
      override_null (m);
      override_null (m, "value");
    }

    bool& valid_;
  };

  // Find special members (id, version).
  //
  struct special_members: traversal::class_, context
  {
    special_members (class_kind_type kind,
                     bool& valid,
                     semantics::data_member*& id,
                     semantics::data_member*& optimistic)
        : kind_ (kind), member_ (valid, id, optimistic)
    {
      if (kind != class_view)
        *this >> inherits_ >> *this;

      *this >> names_ >> member_;
    }

    virtual void
    traverse (semantics::class_& c)
    {
      // Skip transient bases.
      //
      switch (kind_)
      {
      case class_object:
        {
          if (!object (c))
            return;
          break;
        }
      case class_view:
        {
          break;
        }
      case class_composite:
        {
          if (!composite (c))
            return;
          break;
        }
      case class_other:
        {
          assert (false);
          break;
        }
      }

      // Views don't have bases.
      //
      if (kind_ != class_view)
        inherits (c);

      names (c);
    }

  private:
    struct member: traversal::data_member, context
    {
      member (bool& valid,
              semantics::data_member*& id,
              semantics::data_member*& optimistic)
          : valid_ (valid), id_ (id), optimistic_ (optimistic)
      {
      }

      virtual void
      traverse (semantics::data_member& m)
      {
        if (m.count ("id"))
        {
          if (id_ == 0)
            id_ = &m;
          else
          {
            os << m.file () << ":" << m.line () << ":" << m.column () << ":"
               << " error: multiple object id members" << endl;

            os << id_->file () << ":" << id_->line () << ":" << id_->column ()
               << ": info: previous id member is declared here" << endl;

            valid_ = false;
          }
        }

        if (version (m))
        {
          if (optimistic_ == 0)
            optimistic_ = &m;
          else
          {
            os << m.file () << ":" << m.line () << ":" << m.column () << ":"
               << " error: multiple version members" << endl;

            semantics::data_member& o (*optimistic_);

            os << o.file () << ":" << o.line () << ":" << o.column ()
               << ": info: previous version member is declared here" << endl;

            valid_ = false;
          }
        }
      }

      bool& valid_;
      semantics::data_member*& id_;
      semantics::data_member*& optimistic_;
    };

    class_kind_type kind_;
    member member_;
    traversal::names names_;
    traversal::inherits inherits_;
  };

  //
  //
  struct value_type: traversal::type, context
  {
    virtual void
    traverse (semantics::type& t)
    {
      // Resolve null overrides.
      //
      override_null (t);
      override_null (t, "value");
    }
  };

  struct typedefs1: typedefs
  {
    typedefs1 (traversal::declares& d)
        : typedefs (true), declares_ (d)
    {
    }

    void
    traverse (semantics::typedefs& t)
    {
      if (check (t))
        traversal::typedefs::traverse (t);
      else
        declares_.traverse (t);
    }

  private:
    traversal::declares& declares_;
  };

  //
  //
  struct class1: traversal::class_, context
  {
    class1 (bool& valid)
      : valid_ (valid), typedefs_ (declares_), member_ (valid)
    {
      *this >> defines_ >> *this;
      *this >> typedefs_ >> *this;
      *this >> declares_ >> vt_;

      names_member_ >> member_;
    }

    virtual void
    traverse (type& c)
    {
      class_kind_type ck (class_kind (c));

      if (ck != class_other)
      {
        for (type::inherits_iterator i (c.inherits_begin ());
             i != c.inherits_end (); ++i)
        {
          type& b (i->base ());

          if (class_kind (b) == class_other)
            continue;

          location_t cl;
          location_t bl;

          // If we are in the same file, then use real locations (as
          // opposed to definition locations) since that's the order
          // in which we will be generating the code.
          //
          if (class_file (c) == class_file (b))
          {
            cl = class_real_location (c);
            bl = class_real_location (b);
          }
          else
          {
            cl = class_location (c);
            bl = class_location (b);
          }

          if (cl < bl)
          {
            // We cannot use class_name() for b since it might not have
            // yet been processed (tree-hint).
            //
            error (cl) << "base class " << b.name ()  << " must "
                       << "be defined before derived class " << class_name (c)
                       << endl;

            info (bl) << "class " << b.name () << " is define here"
                      << endl;

            valid_ = false;
          }
        }
      }

      switch (ck)
      {
      case class_object:
        names (c);
        traverse_object (c);
        break;
      case class_view:
        names (c);
        traverse_view (c);
        break;
      case class_composite:
        names (c);
        traverse_composite (c);
        // Fall through.
      case class_other:
        vt_.dispatch (c);
        break;
      }
    }

    virtual void
    traverse_object (type& c)
    {
      // Check that the deletion version makes sense.
      //
      if (unsigned long long v = c.get<unsigned long long> ("deleted", 0))
      {
        location_t l (c.get<location_t> ("deleted-location"));

        if (!versioned ())
        {
          error (l) << "deleted class in a non-versioned object model" << endl;
          valid_ = false;
        }
        else
        {
          model_version const& mv (version ());

          if (v > mv.current)
          {
            error (l) << "deletion version is greater than the current " <<
              "model version" << endl;
            valid_ = false;
          }
          else if (v <= mv.base)
          {
            error (l) << "deletion version is less than or equal to the " <<
              "base model version" << endl;
            info (c.location ()) << "delete this class since migration to " <<
              "version " << v << " is no longer possible" << endl;
            valid_ = false;
          }
        }
      }

      // Check that the callback function exist.
      //
      if (c.count ("callback"))
      {
        string name (c.get<string> ("callback"));
        tree decl (
          lookup_qualified_name (
            c.tree_node (), get_identifier (name.c_str ()), false, false));

        if (decl != error_mark_node && TREE_CODE (decl) == BASELINK)
        {
          // Figure out if we have a const version of the callback. OVL_*
          // macros work for both FUNCTION_DECL and OVERLOAD.
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
            if (DECL_CONST_MEMFUNC_P (f))
            {
              c.set ("callback-const", true);
              break;
            }
          }

          //@@ Would be nice to check the signature of the function(s)
          //   instead of postponing it until the C++ compilation. Though
          //   we may still get C++ compilation errors because of const
          //   mismatch.
          //
        }
        else
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ": "
             << "error: unable to resolve member function '" << name << "' "
             << "specified with '#pragma db callback' for class '"
             << class_name (c) << "'" << endl;

          valid_ = false;
        }
      }

      // Check bases.
      //
      type* poly_root (0);

      for (type::inherits_iterator i (c.inherits_begin ());
           i != c.inherits_end ();
           ++i)
      {
        type& b (i->base ());

        if (object (b))
        {
          if (type* r = polymorphic (b))
          {
            if (poly_root == 0)
            {
              poly_root = r;
              c.set ("polymorphic-base", &static_cast<semantics::class_&> (b));
            }
            // If poly_root and r are the same, then we have virtual
            // inheritance. Though we don't support it at the moment.
            //
            else //if (poly_root != r)
            {
              os << c.file () << ":" << c.line () << ":" << c.column () << ":"
                 << " error: persistent class '" << class_name (c) << "' "
                 << "derives from multiple polymorphic bases" << endl;

              type& a (*poly_root);
              os << a.file () << ":" << a.line () << ":" << a.column () << ":"
                 << " info: first polymorphic base defined here" << endl;

              type& b (*r);
              os << b.file () << ":" << b.line () << ":" << b.column () << ":"
                 << " info: second polymorphic base defined here" << endl;

              valid_ = false;
            }
          }
        }
        else if (view (b) || composite (b))
        {
          // @@ Should we use hint here?
          //
          string name (class_fq_name (b));

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: base class '" << name << "' is a view or value type"
             << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: object types cannot derive from view or value "
             << "types"
             << endl;

          os << b.file () << ":" << b.line () << ":" << b.column () << ":"
             << " info: class '" << name << "' is defined here" << endl;

          valid_ = false;
        }
      }

      // Check members.
      //
      names (c, names_member_);

      // Check special members.
      //
      using semantics::data_member;

      data_member* id (0);
      data_member* optimistic (0);
      {
        special_members t (class_object, valid_, id, optimistic);
        t.traverse (c);
      }

      if (id == 0)
      {
        // An object without an id should either be reuse-abstract
        // or explicitly marked as such. We check that it is not
        // polymorphic below.
        //
        if (!(c.count ("id") || abstract (c)))
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: no data member designated as an object id" << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: use '#pragma db id' to specify an object id member"
             << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: or explicitly declare that this persistent class "
             << "has no object id with '#pragma db object no_id'" << endl;

          valid_ = false;
        }
      }
      else
      {
        // Convert id to a member path. This has to happen early since
        // a lot of code that runs next (e.g., processor, pass 1) depends
        // on this information being available.
        //
        data_member_path& idp (c.set ("id-member", data_member_path ()));
        idp.push_back (id);

        // See if we have a member path that we need to resolve.
        //
        const string& name (id->get<string> ("id"));
        location_t l (id->get<location_t> ("id-location"));

        if (!name.empty ())
        {
          if (id->count ("auto"))
          {
            error (l) << "nested id cannot be automatically assigned" << endl;
            valid_ = false;
          }

          if (semantics::class_* comp = utype (*id).is_a<semantics::class_> ())
          {
            try
            {
              resolve_data_members (idp, *comp, name, l, lex_);
            }
            catch (const operation_failed&) {valid_ = false;}
          }
          else
          {
            error (l) << "nested id requires composite member" << endl;
            valid_ = false;
          }

          // Mark the whole member as readonly.
          //
          id->set ("readonly", true);
        }

        data_member* idf (idp.front ());
        data_member* idb (idp.back ());

        // Complain if an id member has a default value (default value
        // for the id's type is ok -- we will ignore it).
        //
        if (idb->count ("default"))
        {
          error (l) << "object id member cannot have default value" << endl;
          valid_ = false;
        }

        // Complain if an id member is in a section.
        //
        if (idf->count ("section-member"))
        {
          error (l) << "object id member cannot be in a section" << endl;
          valid_ = false;
        }

        // Automatically mark the id member as not null. If we already have
        // an explicit null pragma for this member, issue an error.
        //
        if (idb->count ("null"))
        {
          error (l) << "object id member cannot be null" << endl;
          valid_ = false;
        }
        else
          idf->set ("not-null", true);
      }

      if (optimistic != 0)
      {
        semantics::data_member& m (*optimistic);

        // Make sure we have the class declared optimistic.
        //
        if (&m.scope () == &c && !c.count ("optimistic"))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ":"
             << " error: version data member in a class not declared "
             << "optimistic" << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: use '#pragma db optimistic' to declare this "
             << "class optimistic" << endl;

          valid_ = false;
        }

        // Make sure we have object id.
        //
        if (id == 0)
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: optimistic class without an object id" << endl;

          valid_ = false;
        }

        // Make sure id and version members are in the same class. The
        // current architecture relies on that.
        //
        if (id != 0 && &id->scope () != &m.scope ())
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: object id and version members are in different "
             << "classes" << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: object id and version members must be in the same "
             << "class" << endl;

          os << id->file () << ":" << id->line () << ":" << id->column ()
             << ": info: object id member is declared here" << endl;

          os << m.file () << ":" << m.line () << ":" << m.column () << ":"
             << " error: version member is declared here" << endl;

          valid_ = false;
        }

        // Make sure this class is not readonly.
        //
        if (readonly (c))
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: optimistic class cannot be readonly" << endl;

          valid_ = false;
        }

        // Complain if the version member is in a section.
        //
        if (m.count ("section-member"))
        {
          os << m.file () << ":" << m.line () << ":" << m.column ()
             << ": error: version member cannot be in a section" << endl;
          valid_ = false;
        }

        // This takes care of also marking derived classes as optimistic.
        //
        c.set ("optimistic-member", optimistic);
      }
      else
      {
        // Make sure there is a version member if the class declared
        // optimistic.
        //
        if (c.count ("optimistic"))
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: optimistic class without a version member" << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: use '#pragma db version' to declare on of the "
             << "data members as a version" << endl;

          valid_ = false;
        }
      }

      // Polymorphic inheritance.
      //
      if (c.count ("polymorphic") && poly_root == 0)
      {
        // Root of the hierarchy.
        //

        if (id == 0)
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: polymorphic class without an object id" << endl;

          valid_ = false;
        }

        if (!TYPE_POLYMORPHIC_P (c.tree_node ()))
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: non-polymorphic class (class without virtual "
             << "functions) cannot be declared polymorphic" << endl;

          valid_ = false;
        }

        poly_root = &c;
      }

      if (poly_root != 0)
        c.set ("polymorphic-root", poly_root);

      // Sectionable objects.
      //
      if (c.count ("sectionable"))
      {
        if (optimistic == 0)
        {
          location_t l (c.get<location_t> ("sectionable-location"));
          error (l) << "only optimistic class can be sectionable" << endl;
          valid_ = false;
        }
        else if (&optimistic->scope () != &c && poly_root != &c)
        {
          location l (c.get<location_t> ("sectionable-location"));
          error (l) << "only optimistic class that declares the version " <<
            "data member or that is a root of a polymorphic hierarchy can " <<
            "be sectionable" << endl;
          info (optimistic->location ()) << "version member is declared " <<
            "here" << endl;
          valid_ = false;
        }
      }

      // Update features set based on this object.
      //
      if (options.at_once () || class_file (c) == unit.file ())
      {
        if (poly_root != 0)
          features.polymorphic_object = true;
        else if (id == 0 && !abstract (c))
          features.no_id_object = true;
        else
          features.simple_object = true;
      }
    }

    virtual void
    traverse_view (type& c)
    {
      // Views require query support.
      //
      if (!options.generate_query ())
      {
        os << c.file () << ":" << c.line () << ":" << c.column () << ":"
           << " error: query support is required when using views"
           << endl;

        os << c.file () << ":" << c.line () << ":" << c.column () << ":"
           << " info: use the --generate-query option to enable query "
           << "support"
           << endl;

        valid_ = false;
      }

      // Check that the callback function exist.
      //
      if (c.count ("callback"))
      {
        string name (c.get<string> ("callback"));
        tree decl (
          lookup_qualified_name (
            c.tree_node (), get_identifier (name.c_str ()), false, false));

        if (decl == error_mark_node || TREE_CODE (decl) != BASELINK)
        {
          os << c.file () << ":" << c.line () << ":" << c.column () << ": "
             << "error: unable to resolve member function '" << name << "' "
             << "specified with '#pragma db callback' for class '"
             << class_name (c) << "'" << endl;

          valid_ = false;
        }

        // No const version for views.

        //@@ Would be nice to check the signature of the function(s)
        //   instead of postponing it until the C++ compilation. Though
        //   we may still get C++ compilation errors because of const
        //   mismatch.
        //
      }

      // Check bases.
      //
      for (type::inherits_iterator i (c.inherits_begin ());
           i != c.inherits_end ();
           ++i)
      {
        type& b (i->base ());

        if (object (b) || view (b) || composite (b))
        {
          // @@ Should we use hint here?
          //
          string name (class_fq_name (b));

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: base class '" << name << "' is an object, "
             << "view, or value type"
             << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: view types cannot derive from view, object or "
             << "value types"
             << endl;

          os << b.file () << ":" << b.line () << ":" << b.column () << ":"
             << " info: class '" << name << "' is defined here" << endl;

          valid_ = false;
        }
      }

      // Check members.
      //
      names (c, names_member_);

      // Check id.
      //
      semantics::data_member* id (0);
      semantics::data_member* optimistic (0);
      {
        special_members t (class_view, valid_, id, optimistic);
        t.traverse (c);
      }

      if (id != 0)
      {
        os << id->file () << ":" << id->line () << ":" << id->column ()
           << ": error: view type data member cannot be designated as an "
           << "object id" << endl;

        valid_ = false;
      }

      if (optimistic != 0)
      {
        semantics::data_member& o (*optimistic);

        os << o.file () << ":" << o.line () << ":" << o.column ()
           << ": error: view type data member cannot be designated as a "
           << "version" << endl;

        valid_ = false;
      }

      // Update features set based on this view.
      //
      if (options.at_once () || class_file (c) == unit.file ())
      {
        features.view = true;
      }
    }

    virtual void
    traverse_composite (type& c)
    {
      for (type::inherits_iterator i (c.inherits_begin ());
           i != c.inherits_end ();
           ++i)
      {
        type& b (i->base ());

        if (object (b) || view (b))
        {
          // @@ Should we use hint here?
          //
          string name (class_fq_name (b));

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " error: base class '" << name << "' is a view or object "
             << "type"
             << endl;

          os << c.file () << ":" << c.line () << ":" << c.column () << ":"
             << " info: composite value types cannot derive from object "
             << "or view types" << endl;

          os << b.file () << ":" << b.line () << ":" << b.column () << ":"
             << " info: class '" << name << "' is defined here" << endl;

          valid_ = false;
        }
      }

      // Check members.
      //
      names (c, names_member_);

      // Check id.
      //
      semantics::data_member* id (0);
      semantics::data_member* optimistic (0);
      {
        special_members t (class_composite, valid_, id, optimistic);
        t.traverse (c);
      }

      if (id != 0)
      {
        os << id->file () << ":" << id->line () << ":" << id->column ()
           << ": error: value type data member cannot be designated as an "
           << "object id" << endl;

        valid_ = false;
      }

      if (optimistic != 0)
      {
        semantics::data_member& o (*optimistic);

        os << o.file () << ":" << o.line () << ":" << o.column ()
           << ": error: value type data member cannot be designated as a "
           << "version" << endl;

        valid_ = false;
      }
    }

    bool& valid_;

    traversal::defines defines_;
    traversal::declares declares_;
    typedefs1 typedefs_;

    value_type vt_;
    data_member1 member_;
    traversal::names names_member_;

    cxx_string_lexer lex_;
  };

  //
  // Pass 2.
  //

  struct data_member2: traversal::data_member, context
  {
    data_member2 (bool& valid): valid_ (valid) {}

    // Verify that composite value 'c' that is used by data member
    // 'm' is defined before (or inside) class 's'.
    //
    void
    verify_composite_location (semantics::class_& c,
                               semantics::class_& s,
                               semantics::data_member& m)
    {
      if (c.in_scope (s))
        return;

      location_t cl;
      location_t sl;

      // If we are in the same file, then use real locations (as
      // opposed to definition locations) since that's the order
      // in which we will be generating the code.
      //
      if (class_file (c) == class_file (s))
      {
        cl = class_real_location (c);
        sl = class_real_location (s);
      }
      else
      {
        cl = class_location (c);
        sl = class_location (s);
      }

      if (sl < cl)
      {
        const string& cn (class_name (c));

        error (sl)
          << "composite value type " << class_fq_name (c)  << " must "
          << "be defined before being used in class " << class_fq_name (s)
          << endl;

        info (m.location ())
          << "data member " << m.name () << " uses " << cn << endl;

        info (cl) << "class " << cn << " is define here" << endl;

        valid_ = false;
      }
    }

    virtual void
    traverse (type& m)
    {
      using semantics::class_;

      semantics::type& t (utype (m));
      class_& s (dynamic_cast<class_&> (m.scope ()));

      // Validate pointed-to objects.
      //
      class_* c;
      if ((c = object_pointer (t)) || (c = points_to (m)))
      {
        bool poly (polymorphic (*c));
        bool abst (abstract (*c));

        // Make sure the pointed-to class is complete.
        //
        if (!c->complete ())
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": "
             << "error: pointed-to class '" << class_fq_name (*c) << "' "
             << "is incomplete" << endl;

          os << c->file () << ":" << c->line () << ":" << c->column () << ": "
             << "info: class '" << class_name (*c) << "' is declared here"
             << endl;

          os << c->file () << ":" << c->line () << ":" << c->column () << ": "
             << "info: consider including its definition with the "
             << "--odb-epilogue option" << endl;

          valid_ = false;
          return;
        }

        // Make sure the pointed-to class is not reuse-abstract.
        //
        if (abst && !poly)
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": "
             << "error: pointed-to class '" << class_fq_name (*c) << "' "
             << "is abstract" << endl;

          os << c->file () << ":" << c->line () << ":" << c->column () << ": "
             << "info: class '" << class_name (*c) << "' is defined here"
             << endl;

          throw operation_failed ();
        }

        // Make sure the pointed-to class has object id unless it is in a
        // view where we can load no-id objects.
        //
        if (data_member_path* id = id_member (*c))
        {
          semantics::type& idt (utype (*id));

          // Make sure composite id is defined before this pointer. Failed
          // that we get non-obvious C++ compiler errors in generated code.
          //
          if (class_* comp = composite_wrapper (idt))
            verify_composite_location (*comp, s, m);
        }
        else if (!view_member (m))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": "
             << "error: pointed-to class '" << class_fq_name (*c) << "' "
             << "has no object id" << endl;

          os << c->file () << ":" << c->line () << ":" << c->column () << ": "
             << "info: class '" << class_name (*c) << "' is defined here"
             << endl;

          valid_ = false;
          return;
        }

        // Make sure the pointed-to class has a default ctor. Since we will
        // use database::load() in the generated code, lack of a default ctor
        // will lead to uncompilable generated code. Poly-abstract is Ok.
        //
        if (!c->default_ctor () && !(abst && poly))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": "
             << "error: pointed-to class '" << class_fq_name (*c) << "' "
             << "has no default constructor" << endl;

          os << c->file () << ":" << c->line () << ":" << c->column () << ": "
             << "info: class '" << class_name (*c) << "' is defined here"
             << endl;

          valid_ = false;
          return;
        }
      }

      // Make sure composite type is defined before (or inside)
      // this class. Failed that we get non-obvious C++ compiler
      // errors in generated code.
      //
      if (class_* comp = composite_wrapper (t))
        verify_composite_location (*comp, s, m);

      // Check that containers with composite value element types don't have
      // any nested containers.
      //
      if (semantics::type* c = container (m))
      {
        class_* vt (0);
        class_* kt (0);

        switch (container_kind (*c))
        {
        case ck_map:
        case ck_multimap:
          {
            kt = composite_wrapper (container_kt (m));
          }
          // Fall through.
        default:
          {
            vt = composite_wrapper (container_vt (m));
          }
        }

        if (vt != 0 && has_a (*vt, test_container))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": "
             << "error: containers of containers not supported" << endl;

          os << vt->file () << ":" << vt->line () << ":" << vt->column ()
             << ": info: composite element value " << class_fq_name (*vt)
             << " contains container(s)" << endl;

          valid_ = false;
        }

        if (kt != 0 && has_a (*kt, test_container))
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ": "
             << "error: containers of containers not supported" << endl;

          os << kt->file () << ":" << kt->line () << ":" << kt->column ()
             << ": info: composite element key " << class_fq_name (*kt)
             << " contains container(s)" << endl;

          valid_ = false;
        }
      }
    }

    bool& valid_;
  };

  // Make sure soft-delete versions make sense for dependent entities.
  // We don't seem to need anything for soft-add since if an entity is
  // not added (e.g., an object), then we cannot reference it in the
  // C++ sense (e.g., a pointer).
  //
  struct version_dependencies: object_members_base
  {
    version_dependencies (bool& valid): valid_ (valid) {}

    virtual void
    traverse_object (semantics::class_& c)
    {
      // For reuse inheritance we allow the base to be soft-deleted. In
      // a sense, it becomes like an abstract class with the added
      // functionality of being able to load old data.
      //
      // For polymorphism inheritance things are a lot more complicated
      // and we don't allow a base to be soft-deleted since there is a
      // link (foreign key) from the derived table.
      //
      semantics::class_* poly (polymorphic (c));
      if (poly != 0 && poly != &c)
      {
        semantics::class_& base (polymorphic_base (c));
        check_strict (
          c, base, "polymorphic derived object", "polymorphic base");
      }

      // Don't go into bases.
      //
      names (c);
    }

    virtual void
    traverse_simple (semantics::data_member& m)
    {
      semantics::class_& c (dynamic_cast<semantics::class_&> (m.scope ()));

      switch (class_kind (c))
      {
      case class_object:
        {
          // Member shouldn't be deleted after the object that contains it.
          //
          check_lax (m, c, "data member", "object");
          break;
        }
        // We leave it up to the user to make sure the view is consistent
        // with the database schema it is being run on.
        //
      default:
        break;
      }
    }

    virtual void
    traverse_composite (semantics::data_member* m, semantics::class_& c)
    {
      if (m != 0)
        traverse_simple (*m); // Do simple value tests.
      else
        names (c); // Don't go into bases.
    }

    virtual void
    traverse_pointer (semantics::data_member& m, semantics::class_& c)
    {
      traverse_simple (m); // Do simple value tests.

      // Pointer must be deleted before the pointed-to object.
      //
      check_strict (m, c, "object pointer", "pointed-to object");

      // Inverse pointer must be deleted before the direct side.
      //
      if (data_member_path* imp = inverse (m))
        check_strict (m, *imp, "inverse object pointer", "direct pointer");
    }

    virtual void
    traverse_container (semantics::data_member& m, semantics::type&)
    {
      traverse_simple (m); // Do simple value tests.

      if (semantics::class_* c = object_pointer (container_vt (m)))
      {
        // Pointer must be deleted before the pointed-to object.
        //
        check_strict (m, *c, "object pointer", "pointed-to object");

        // Inverse pointer must be deleted before the direct side.
        //
        if (data_member_path* imp = inverse (m, "value"))
          check_strict (m, *imp, "inverse object pointer", "direct pointer");
      }
    }

  private:
    // Check for harmless things like a data member deleted after the
    // object.
    //
    template <typename D, typename P>
    void
    check_lax (D& dep, P& pre, char const* dname, char const* pname)
    {
      unsigned long long dv (deleted (dep));
      unsigned long long pv (deleted (pre));

      if (pv == 0 || // Prerequisite never deleted.
          dv == 0 || // Dependent never deleted.
          dv <= pv)  // Dependent deleted before prerequisite.
        return;

      location_t dl (dep.template get<location_t> ("deleted-location"));
      location_t pl (pre.template get<location_t> ("deleted-location"));

      error (dl) << dname << " is deleted after " << dname << endl;
      info (pl) << pname << " deletion version is specified here" << endl;

      valid_ = false;
    }

    // Check for harmful things that will result in an invalid database
    // schema, like a pointer pointing to a deleted object.
    //
    template <typename D, typename P>
    void
    check_strict (D& dep, P& pre, char const* dname, char const* pname)
    {
      location_t dl (0), pl (0);
      unsigned long long dv (deleted (dep, &dl));
      unsigned long long pv (deleted (pre, &pl));

      if (pv == 0) // Prerequisite never deleted.
        return;

      if (dv == 0) // Dependent never deleted.
      {
        location dl (dep.location ());

        error (dl) << dname << " is not deleted" << endl;
        info (pl) << pname << " is deleted here" << endl;

        valid_ = false;
      }
      else if (pv > dv) // Prerequisite deleted before dependent.
      {
        error (dl) << dname << " is deleted after " << pname << endl;
        info (pl) << pname << " deletion version is specified here" << endl;

        valid_ = false;
      }
    }

  private:
    bool& valid_;
  };

  struct class2: traversal::class_, context
  {
    class2 (bool& valid)
    : valid_ (valid), has_lt_operator_ (0), typedefs_ (true), member_ (valid)
    {
      // Find the has_lt_operator function template.
      //
      tree odb (
        lookup_qualified_name (
          global_namespace, get_identifier ("odb"), false, false));

      if (odb != error_mark_node)
      {
        tree compiler (
          lookup_qualified_name (
            odb, get_identifier ("compiler"), false, false));

        if (compiler != error_mark_node)
        {
          has_lt_operator_ = lookup_qualified_name (
            compiler, get_identifier ("has_lt_operator"), false, false);

          if (has_lt_operator_ != error_mark_node)
          {
#if BUILDING_GCC_MAJOR >= 8
            has_lt_operator_ = OVL_FIRST (has_lt_operator_);
#else
            has_lt_operator_ = OVL_CURRENT (has_lt_operator_);
#endif
          }
          else
          {
            os << unit.file () << ": error: unable to resolve has_lt_operator "
               << "function template inside odb::compiler" << endl;
            has_lt_operator_ = 0;
          }
        }
        else
          os << unit.file () << ": error: unable to resolve compiler "
             << "namespace inside odb" << endl;
      }
      else
        os << unit.file () << ": error: unable to resolve odb namespace"
           << endl;

      if (has_lt_operator_ == 0)
        valid_ = false;

      *this >> defines_ >> *this;
      *this >> typedefs_ >> *this;

      names_member_ >> member_;
    }

    virtual void
    traverse (type& c)
    {
      class_kind_type ck (class_kind (c));

      if (ck == class_other)
        return;

      names (c);

      switch (ck)
      {
      case class_object: traverse_object (c); break;
      case class_view: traverse_view (c); break;
      case class_composite: traverse_composite (c); break;
      default: return;
      }

      // Check members.
      //
      names (c, names_member_);

      // Check version dependencies.
      //
      {
        version_dependencies vd (valid_);
        vd.traverse (c);
      }
    }

    virtual void
    traverse_object (type& c)
    {
      bool abst (abstract (c));
      bool poly (polymorphic (c));

      // Make sure we have no empty or pointless sections unless we
      // are reuse-abstract or polymorphic.
      //
      if (!poly && !abst)
      {
        user_sections& uss (c.get<user_sections> ("user-sections"));

        for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
        {
          user_section& s (*i);

          // Skip the special version update section (we always treat it
          // as abstract in reuse inheritance).
          //
          if (s.special == user_section::special_version)
            continue;

          semantics::data_member& m (*s.member);
          location const& l (m.location ());

          if (s.total == 0 && !s.containers)
          {
            error (l) << "empty section" << endl;

            if (&m.scope () != &c)
              info (c.location ()) << "as seen in this non-abstract " <<
                "persistent class" << endl;

            valid_ = false;
            continue;
          }

          // Eager-loaded section with readonly members.
          //
          if (s.load == user_section::load_eager && s.update_empty ())
          {
            error (l) << "eager-loaded section with readonly members is " <<
              "pointless" << endl;

            if (&m.scope () != &c)
              info (c.location ()) << "as seen in this non-abstract " <<
                "persistent class" << endl;

            valid_ = false;
          }
        }
      }

      if (data_member_path* id = id_member (c))
      {
        semantics::type& t (utype (*id));

        // If this is a session object, make sure that the id type can
        // be compared.
        //
        if (session (c) && has_lt_operator_ != 0)
        {
          tree args (make_tree_vec (1));
          TREE_VEC_ELT (args, 0) = t.tree_node ();

          tree inst (
            instantiate_template (
              has_lt_operator_, args, tf_none));

          bool v (inst != error_mark_node);

          // Old version of has_lt_operator() relied on full instantiation
          // while the new one is based on SFINAE.
          //
#if 0
          if (v &&
              DECL_TEMPLATE_INSTANTIATION (inst) &&
              !DECL_TEMPLATE_INSTANTIATED (inst))
          {
            // Instantiate this function template to see if the value type
            // provides operator<. Unfortunately, GCC instantiate_decl()
            // does not provide any control over the diagnostics it issues
            // in case of an error. To work around this, we are going to
            // temporarily redirect diagnostics to /dev/null, which is
            // where asm_out_file points to (see plugin.cxx).
            //
            // Needless to say, this is very hacky and we should quickly fail
            // (as we do below) if there were errors.
            //
            int ec (errorcount);
            FILE* s (global_dc->printer->buffer->stream);
            global_dc->printer->buffer->stream = asm_out_file;

            instantiate_decl (inst, false, false);

            global_dc->printer->buffer->stream = s;
            v = (ec == errorcount);
          }
#endif

          if (!v)
          {
            semantics::data_member& idm (*id->front ());

            os << t.file () << ":" << t.line () << ":" << t.column ()
               << ": error: value type that is used as object id in "
               << "persistent class with session support does not define "
               << "the less than (<) comparison" << endl;

            os << t.file () << ":" << t.line () << ":" << t.column ()
               << ": info: provide operator< for this value type" << endl;

            os << idm.file () << ":" << idm.line () << ":" << idm.column ()
               << ": info: id member is defined here" << endl;

            os << c.file () << ":" << c.line () << ":" << c.column ()
               << ": info: persistent class is defined here" << endl;

            valid_ = false;
          }
        }
      }
      else
      {
        // Make sure an object without id has no sections.
        //
        user_sections& uss (c.get<user_sections> ("user-sections"));

        if (!uss.empty ())
        {
          semantics::data_member& m (*uss.front ().member);
          os << m.file () << ":" << m.line () << ":" << m.column ()
             << ": error: object without id cannot have sections" << endl;
          valid_ = false;
        }
      }

      // Allow all the members to be deleted as long as there is no
      // schema for this class.
      //
      column_count_type const& cc (column_count (c));
      size_t cont (has_a (c, test_container));
      size_t dcont (cont - has_a (c, test_container | exclude_deleted));

      if ((cc.total == 0 && cont == 0) ||
          (cc.total == cc.deleted && cont == dcont && !(abst || deleted (c))))
      {
        os << c.file () << ":" << c.line () << ":" << c.column () << ":"
           << " error: no persistent data members in the class" << endl;
        valid_ = false;
      }
    }

    virtual void
    traverse_view (type& c)
    {
      // We don't check for the column count here since we may want to
      // allow certain kinds of empty views. Instead, this is handled
      // in relational::validation.

      // See if any of the associated objects are polymorphic. If so,
      // we need to include polymorphism-specific headers. Also, if the
      // view is loading the object, then we will need the corresponding
      // statements.
      //
      if (c.count ("objects"))
      {
        view_objects& objs (c.get<view_objects> ("objects"));

        for (view_objects::iterator i (objs.begin ()); i != objs.end (); ++i)
        {
          if (i->kind == view_object::object)
          {
            if (polymorphic (*i->obj))
              features.polymorphic_object = true;
            else if (i->ptr != 0)
            {
              (id_member (*i->obj) != 0
               ? features.simple_object
               : features.no_id_object) = true;
            }
          }
        }
      }
    }

    virtual void
    traverse_composite (type& c)
    {
      // Allow all the members to be deleted.
      //
      column_count_type const& cc (column_count (c));
      size_t cont (has_a (c, test_container));

      if (cc.total == 0 && cont == 0)
      {
        os << c.file () << ":" << c.line () << ":" << c.column () << ":"
           << " error: no persistent data members in the class" << endl;
        valid_ = false;
      }
    }

    bool& valid_;
    tree has_lt_operator_;

    traversal::defines defines_;
    typedefs typedefs_;

    data_member2 member_;
    traversal::names names_member_;
  };
}

void
validate (options const& ops,
          features& f,
          semantics::unit& u,
          semantics::path const& p,
          unsigned short pass)
{
  bool valid (true);
  database db (ops.database ()[0]);

  // Validate options.
  //
  if (ops.generate_schema_only () &&
      ops.schema_format ()[db].count (schema_format::embedded))
  {
    cerr << "error: --generate-schema-only is only valid when generating " <<
      "schema as a standalone SQL or separate C++ file" << endl;
    valid = false;
  }

  // Multi-database support options.
  //
  if (ops.multi_database () == multi_database::dynamic &&
      ops.default_database_specified () &&
      ops.default_database () != database::common)
  {
    cerr << "error: when dynamic multi-database support is used, the " <<
      "default database can only be 'common'" << endl;
    valid = false;
  }

  if (db == database::common &&
      ops.multi_database () == multi_database::disabled)
  {
    cerr << "error: 'common' database is only valid with multi-database " <<
      "support enabled" << endl;
    valid = false;
  }

  // Changelog options.
  //
  if (ops.changelog_in ().count (db) != ops.changelog_out ().count (db))
  {
    cerr << "error: both --changelog-in and --changelog-out must be " <<
      "specified" << endl;
    valid = false;
  }

  if (!valid)
    throw validator_failed ();

  unique_ptr<context> ctx (create_context (cerr, u, ops, f, 0));

  if (pass == 1)
  {
    traversal::unit unit;
    traversal::defines unit_defines;
    traversal::declares unit_declares;
    typedefs1 unit_typedefs (unit_declares);
    traversal::namespace_ ns;
    value_type vt;
    class1 c (valid);

    unit >> unit_defines >> ns;
    unit_defines >> c;
    unit >> unit_declares >> vt;
    unit >> unit_typedefs >> c;

    traversal::defines ns_defines;
    traversal::declares ns_declares;
    typedefs1 ns_typedefs (ns_declares);

    ns >> ns_defines >> ns;
    ns_defines >> c;
    ns >> ns_declares >> vt;
    ns >> ns_typedefs >> c;

    unit.dispatch (u);
  }
  else
  {
    traversal::unit unit;
    traversal::defines unit_defines;
    typedefs unit_typedefs (true);
    traversal::namespace_ ns;
    class2 c (valid);

    unit >> unit_defines >> ns;
    unit_defines >> c;
    unit >> unit_typedefs >> c;

    traversal::defines ns_defines;
    typedefs ns_typedefs (true);

    ns >> ns_defines >> ns;
    ns_defines >> c;
    ns >> ns_typedefs >> c;

    unit.dispatch (u);
  }

  if (!valid)
    throw validator_failed ();

  switch (db)
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
      try
      {
        relational::validate (ops, f, u, p, pass);
      }
      catch (operation_failed const&)
      {
        throw validator_failed ();
      }

      break;
    }
  }
}
