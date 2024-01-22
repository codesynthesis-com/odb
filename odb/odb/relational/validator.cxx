// file      : odb/relational/validator.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <map>
#include <iostream>

#include <odb/diagnostics.hxx>
#include <odb/traversal.hxx>
#include <odb/relational/common.hxx>
#include <odb/relational/context.hxx>
#include <odb/relational/validator.hxx>

using namespace std;

namespace relational
{
  namespace
  {
    //
    // Pass 2.
    //

    struct data_member2: traversal::data_member, context
    {
      data_member2 (bool& valid)
          : valid_ (valid)
      {
      }

      virtual void
      traverse (type& m)
      {
        if (transient (m))
          return;

        if (null (m))
        {
          if (semantics::class_* c = composite_wrapper (utype (m)))
          {
            if (has_a (*c, test_container))
            {
              os << m.file () << ":" << m.line () << ":" << m.column () << ":"
                 << " error: composite member containing containers cannot "
                 << "be null" << endl;

              os << c->file () << ":" << c->line () << ":" << c->column ()
                 << ": info: composite value type is defined here" << endl;

              valid_ = false;
            }
          }
        }

        // Check on-delete.
        //
        if (m.count ("on-delete"))
        {
          const char* kp (container (m) ? "value" : "");
          location l (m.location ());

          // Make sure it is a pointer or a member with points_to pragma.
          //
          if (!object_pointer (utype (m, kp)) && !points_to (m))
          {
            error (l) << "on_delete specified for non-object pointer" << endl;
            valid_ = false;
          }

          // Make sure it is not inverse.
          //
          if (inverse (m, kp))
          {
            error (l) << "on_delete specified for inverse object " <<
              "pointer" << endl;
            valid_ = false;
          }

          // Make sure the pointer is nullable if asked to set it to NULL.
          //
          using sema_rel::foreign_key;

          if (m.get<foreign_key::action_type> ("on-delete") ==
              foreign_key::set_null &&
              !null (m, kp))
          {
            error (l) << "set_null specified for non-nullable object "
              "pointer" << endl;
            valid_ = false;
          }
        }
      }

      bool& valid_;
    };

    struct object_no_id_members: object_members_base
    {
      object_no_id_members (bool& valid)
          : object_members_base (false, false, true), valid_ (valid), dm_ (0)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_&)
      {
        if (inverse (m))
        {
          semantics::data_member& dm (dm_ != 0 ? *dm_ : m);

          os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
             << " error: inverse object pointer member '" << member_prefix_
             << m.name () << "' in an object without an object id" << endl;

          valid_ = false;
        }
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type&)
      {
        semantics::data_member& dm (dm_ != 0 ? *dm_ : m);

        os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
           << " error: container member '" << member_prefix_ << m.name ()
           << "' in an object without an object id" << endl;

        valid_ = false;
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        semantics::data_member* old_dm (dm_);

        if (dm_ == 0)
          dm_ = m;

        object_members_base::traverse_composite (m, c);

        dm_ = old_dm;
      }

    private:
      bool& valid_;
      semantics::data_member* dm_; // Direct object data member.
    };

    struct composite_id_members: object_members_base
    {
      composite_id_members (bool& valid)
          : object_members_base (false, false, true), valid_ (valid), dm_ (0)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_&)
      {
        semantics::data_member& dm (dm_ != 0 ? *dm_ : m);

        os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
           << " error: object pointer member '" << member_prefix_ << m.name ()
           << "' in a composite value type that is used as an object id"
           << endl;

        valid_ = false;
      }

      virtual void
      traverse_simple (semantics::data_member& m)
      {
        if (readonly (member_path_, member_scope_))
        {
          semantics::data_member& dm (dm_ != 0 ? *dm_ : m);

          os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
             << " error: readonly member '" << member_prefix_ << m.name ()
             << "' in a composite value type that is used as an object id"
             << endl;

          valid_ = false;
        }
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type&)
      {
        semantics::data_member& dm (dm_ != 0 ? *dm_ : m);

        os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
           << " error: container member '" << member_prefix_ << m.name ()
           << "' in a composite value type that is used as an object id"
           << endl;

        valid_ = false;
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        semantics::data_member* old_dm (dm_);

        if (dm_ == 0)
          dm_ = m;

        object_members_base::traverse_composite (m, c);

        dm_ = old_dm;
      }

    private:
      bool& valid_;
      semantics::data_member* dm_; // Direct composite member.
    };

    struct view_members: object_members_base
    {
      view_members (bool& valid)
          : object_members_base (false, false, true), valid_ (valid), dm_ (0)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_&)
      {
        if (dm_ != 0 && object_pointer (utype (m)))
        {
          location const& l (dm_->location ());

          error (l) << "nested view data member '" << member_prefix_
                    << m.name () << "' is an object pointer" << endl;
          info (l) << "views can only contain direct object pointer members"
                   << endl;

          valid_ = false;
        }
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type&)
      {
        semantics::data_member& dm (dm_ != 0 ? *dm_ : m);

        os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
           << " error: view data member '" << member_prefix_ << m.name ()
           << "' is a container" << endl;

        os << dm.file () << ":" << dm.line () << ":" << dm.column () << ":"
           << ": info: views cannot contain containers" << endl;

        valid_ = false;
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        semantics::data_member* old_dm (dm_);

        if (dm_ == 0)
          dm_ = m;

        object_members_base::traverse_composite (m, c);

        dm_ = old_dm;
      }

    private:
      bool& valid_;
      semantics::data_member* dm_; // Direct view data member.
    };

    struct class2: traversal::class_, context
    {
      class2 (bool& valid)
          : valid_ (valid),
            typedefs_ (true),
            data_member_ (valid),
            object_no_id_members_ (valid),
            composite_id_members_ (valid),
            view_members_ (valid)
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;

        data_member_names_ >> data_member_;
      }

      virtual void
      traverse (type& c)
      {
        class_kind_type ck (class_kind (c));
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
          break;
        case class_other:
          break;
        }

        // Make sure indexes are not defined for anything other than objects.
        //
        if (c.count ("index") && ck != class_object)
        {
          indexes& ins (c.get<indexes> ("index"));

          for (indexes::iterator i (ins.begin ()); i != ins.end (); ++i)
          {
            error (i->loc) << "index definition on a non-persistent class"
                           << endl;
            valid_ = false;
          }
        }
      }

      virtual void
      traverse_object (type& c)
      {
        data_member_path* id (id_member (c));

        if (id != 0)
        {
          if (semantics::class_* cm = composite_wrapper (utype (*id)))
          {
            location idl (id->front ()->location ());

            // Composite id cannot be auto.
            //
            if (auto_ (*id))
            {
              error (idl) << "composite id cannot be automatically assigned"
                          << endl;
              valid_ = false;
            }

            // Make sure we don't have any containers or pointers in this
            // composite value type.
            //
            if (valid_)
            {
              composite_id_members_.traverse (*cm);

              if (!valid_)
                info (idl) << "composite id is defined here" << endl;
            }

            // Check that the composite value type is default-constructible.
            //
            if (!cm->default_ctor ())
            {
              os << cm->file () << ":" << cm->line () << ":" << cm->column ()
                 << ": error: composite value type that is used as object id "
                 << "is not default-constructible" << endl;

              os << cm->file () << ":" << cm->line () << ":" << cm->column ()
                 << ": info: provide default constructor for this value type"
                 << endl;

              info (idl) << "composite id is defined here" << endl;

              valid_ = false;
            }
          }
        }
        else
        {
          if (!abstract (c))
          {
            // Make sure we don't have any containers or inverse pointers.
            //
            object_no_id_members_.traverse (c);
          }
        }

        names (c, data_member_names_);

        // Validate bulk operation support.
        //
        for (bool i (true); i && c.count ("bulk"); i = false)
        {
          location_t l (c.get<location_t> ("bulk-location"));

          if (polymorphic (c))
          {
            error (l) << "bulk operations on polymorphic objects are "
              "not supported" << endl;
            valid_ = false;
            break;
          }

          if (has_a (c, test_straight_container))
          {
            error (l) << "bulk operations on objects with containers are "
              "not supported" << endl;
            valid_ = false;
            break;
          }

          bool update (true);

          // Unless we only have manually-updated sections, we cannot generate
          // the bulk update operation.
          //
          user_sections& uss (c.get<user_sections> ("user-sections"));

          for (user_sections::iterator i (uss.begin ());
               update && i != uss.end ();
               ++i)
          {
            const user_section& s (*i);

            // Skip special sections.
            //
            if (s.special != user_section::special_ordinary)
              continue;

            // Always-updated section still needs a separate statement
            // (since it may not be loaded).
            //
            if (!s.update_empty () && s.update != user_section::update_manual)
              update = false;
          }

          c.set ("bulk-persist", true);
          if (update) c.set ("bulk-update", true);
          c.set ("bulk-erase", true);
        }

        // Validate indexes.
        //
        {
          indexes& ins (c.get<indexes> ("index"));

          // Make sure index members are not transient, inverse, or
          // containers.
          //
          for (indexes::iterator i (ins.begin ()); i != ins.end (); ++i)
          {
            index& in (*i);

            for (index::members_type::iterator i (in.members.begin ());
                 i != in.members.end (); ++i)
            {
              index::member& im (*i);
              semantics::data_member& m (*im.path.back ());

              if (transient (m))
              {
                error (im.loc) << "index member is transient" << endl;
                valid_ = false;
              }

              if (inverse (m))
              {
                error (im.loc) << "index member is an inverse object " <<
                  "pointer" << endl;
                valid_ = false;
              }

              if (container (m))
              {
                error (im.loc) << "index member is a container" << endl;
                valid_ = false;
              }
            }
          }
        }
      }

      virtual void
      traverse_view (type& c)
      {
        const view_query& vq (c.get<view_query> ("query"));

        // Make sure we don't have any containers or object pointers.
        //
        view_members_.traverse (c);

        names (c, data_member_names_);

        // Allow certain kinds of empty views.
        //
        if (vq.kind != view_query::runtime &&
            vq.kind != view_query::complete_execute)
        {
          // Allow all the members to be deleted.
          //
          column_count_type const& cc (column_count (c));

          if (cc.total == 0)
          {
            os << c.file () << ":" << c.line () << ":" << c.column () << ":"
               << " error: no persistent data members in the class" << endl;
            valid_ = false;
          }
        }
      }

      virtual void
      traverse_composite (type& c)
      {
        names (c, data_member_names_);
      }

    public:
      bool& valid_;

      traversal::defines defines_;
      typedefs typedefs_;

      data_member2 data_member_;
      traversal::names data_member_names_;

      object_no_id_members object_no_id_members_;
      composite_id_members composite_id_members_;
      view_members view_members_;
    };
  }

  void
  validate (options const&,
            features&,
            semantics::unit& u,
            semantics::path const&,
            unsigned short pass)
  {
    bool valid (true);

    // Validate custom type mapping.
    //
    if (pass == 1)
    {
      // Create an empty list if we don't have one. This makes the
      // rest of the code simpler.
      //
      if (!u.count ("custom-db-types"))
        u.set ("custom-db-types", custom_db_types ());

      custom_db_types & cts (u.get<custom_db_types> ("custom-db-types"));

      for (custom_db_types::iterator i (cts.begin ()); i != cts.end (); ++i)
      {
        custom_db_type& ct (*i);

        if (ct.type.empty ())
        {
          error (ct.loc) << "'type' clause expected in db pragma map" << endl;
          valid = false;
        }

        if (ct.as.empty ())
        {
          error (ct.loc) << "'as' clause expected in db pragma map" << endl;
          valid = false;
        }

        if (ct.to.empty ())
          ct.to = "(?)";
        else
        {
          size_t p (ct.to.find ("(?)"));

          if (p == string::npos)
          {
            error (ct.loc) << "no '(?)' expression in the 'to' clause "
                           << "of db pragma map" << endl;
            valid = false;
          }
          else if (ct.to.find ("(?)", p + 3) != string::npos)
          {
            error (ct.loc) << "multiple '(?)' expressions in the 'to' "
                           << "clause of db pragma map" << endl;
            valid = false;
          }
        }

        if (ct.from.empty ())
          ct.from = "(?)";
        else
        {
          size_t p (ct.from.find ("(?)"));

          if (p == string::npos)
          {
            error (ct.loc) << "no '(?)' expression in the 'from' clause "
                           << "of db pragma map" << endl;
            valid = false;
          }
          else if (ct.from.find ("(?)", p + 3) != string::npos)
          {
            error (ct.loc) << "multiple '(?)' expressions in the 'from' "
                           << "clause of db pragma map" << endl;
            valid = false;
          }
        }
      }
    }

    if (!valid)
      throw operation_failed ();

    if (pass == 1)
    {
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
      throw operation_failed ();
  }
}
