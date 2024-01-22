// file      : odb/relational/processor.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <vector>
#include <algorithm>

#include <odb/diagnostics.hxx>
#include <odb/lookup.hxx>
#include <odb/cxx-lexer.hxx>
#include <odb/common.hxx>

#include <odb/relational/context.hxx>
#include <odb/relational/processor.hxx>

using namespace std;

namespace relational
{
  namespace
  {
    // Indirect (dynamic) context values.
    //
    static string
    id_column_type ()
    {
      context& c (context::current ());
      data_member_path& id (*context::id_member (*c.top_object));
      return id.back ()->get<string> ("column-id-type");
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

        semantics::type* wt;
        semantics::names* whint (0);
        if ((wt = wrapper (t, whint)))
          wt = &utype (*wt, whint);

        // Determine the member kind.
        //
        enum {simple, composite, container, unknown} kind (unknown);

        // See if this is a composite value type.
        //
        if (composite_wrapper (t))
          kind = composite;

        // If not, see if it is a simple value.
        //
        if (kind == unknown)
        {
          string type, id_type;

          if (m.count ("id-type"))
            id_type = m.get<string> ("id-type");

          if (m.count ("type"))
          {
            type = m.get<string> ("type");

            if (id_type.empty ())
              id_type = type;
          }

          if (semantics::class_* c = object_pointer (t))
          {
            // An object pointer in view doesn't really have a "column"
            // so pretend that it has already been handled.
            //
            if (view_member (m))
              kind = simple;
            else
            {
              // This is an object pointer. The column type is the pointed-to
              // object id type.
              //
              semantics::data_member& id (*id_member (*c)->back ());

              semantics::names* idhint;
              semantics::type& idt (utype (id, idhint));

              // The id type can be a composite value type.
              //
              if (composite_wrapper (idt))
                kind = composite;
              else
              {
                semantics::type* wt;
                semantics::names* whint (0);
                if ((wt = wrapper (idt, whint)))
                  wt = &utype (*wt, whint);

                if (type.empty () && id.count ("id-type"))
                  type = id.get<string> ("id-type");

                if (type.empty () && id.count ("type"))
                  type = id.get<string> ("type");

                // The rest should be identical to the code for the id_type in
                // the else block.
                //
                if (type.empty () && idt.count ("id-type"))
                  type = idt.get<string> ("id-type");

                if (type.empty () && wt != 0 && wt->count ("id-type"))
                  type = wt->get<string> ("id-type");

                if (type.empty () && idt.count ("type"))
                  type = idt.get<string> ("type");

                if (type.empty () && wt != 0 && wt->count ("type"))
                  type = wt->get<string> ("type");

                if (type.empty ())
                  type = database_type (idt, idhint, true);

                if (type.empty () && wt != 0)
                  type = database_type (*wt, whint, true);

                id_type = type;
              }
            }
          }
          else
          {
            if (id_type.empty () && t.count ("id-type"))
              id_type = t.get<string> ("id-type");

            if (id_type.empty () && wt != 0 && wt->count ("id-type"))
              id_type = wt->get<string> ("id-type");

            if (type.empty () && t.count ("type"))
              type = t.get<string> ("type");

            if (type.empty () && wt != 0 && wt->count ("type"))
              type = wt->get<string> ("type");

            if (id_type.empty ())
              id_type = type;

            if (id_type.empty ())
              id_type = database_type (t, hint, true);

            if (id_type.empty () && wt != 0)
              id_type = database_type (*wt, whint, true);

            bool null (false);
            if (type.empty ())
              type = database_type (t, hint, false, &null);

            if (type.empty () && wt != 0)
              type = database_type (*wt, whint, false, &null);

            // Use id mapping for discriminators.
            //
            if (id (m) || discriminator (m))
              type = id_type;
            // Allow NULL if requested by the default mapping.
            //
            else if (null && !m.count ("not-null"))
              m.set ("null", true);
          }

          if (kind == unknown && !type.empty ())
          {
            m.set ("column-type", type);
            m.set ("column-id-type", id_type);

            // Issue a warning if we are relaxing null-ness.
            //
            if (m.count ("null") && t.count ("not-null"))
            {
              os << m.file () << ":" << m.line () << ":" << m.column () << ":"
                 << " warning: data member declared null while its type is "
                 << "declared not null" << endl;
            }

            kind = simple;
          }
        }

        // If not a simple value, see if this is a container.
        //
        if (kind == unknown && context::container (m))
        {
          process_container (m, (wt != 0 ? *wt : t));
          kind = container;
        }

        // If it is none of the above then we have an error.
        //
        if (kind == unknown)
        {
          os << m.file () << ":" << m.line () << ":" << m.column () << ":"
             << " error: unable to map C++ type '" << t.fq_name (hint)
             << "' used in data member '" << m.name () << "' to a "
             << db.name () << " database type" << endl;

          os << m.file () << ":" << m.line () << ":" << m.column () << ":"
             << " info: use '#pragma db type' to specify the database type"
             << endl;

          throw operation_failed ();
        }

        if (m.count ("polymorphic-ref"))
        {
          // Copy the column name from the root's id member, if specified.
          //
          {
            semantics::class_& r (*object_pointer (t));
            semantics::data_member& id (*id_member (r)->front ());

            if (id.count ("column"))
              m.set ("column", id.get<table_column> ("column"));
          }

          m.set ("not-null", true);
          m.set ("deferrable",
                 sema_rel::deferrable (sema_rel::deferrable::not_deferrable));
          m.set ("on-delete", sema_rel::foreign_key::cascade);
        }

        process_index (m);
      }

      // Convert index/unique specifiers to the index entry in the object.
      //
      void
      process_index (semantics::data_member& m)
      {
        bool ip (m.count ("index"));
        bool up (m.count ("unique"));

        if (ip || up)
        {
          using semantics::class_;
          class_& c (dynamic_cast<class_&> (m.scope ()));

          indexes& ins (c.count ("index")
                        ? c.get<indexes> ("index")
                        : c.set ("index", indexes ()));

          index in;
          in.loc = m.get<location_t> (
            ip ? "index-location" : "unique-location");

          if (up)
            in.type = "UNIQUE";

          index::member im;
          im.loc = in.loc;
          im.name = m.name ();
          im.path.push_back (&m);
          in.members.push_back (im);

          // Insert it in the location order.
          //
          ins.insert (
            lower_bound (ins.begin (), ins.end (), in, index_comparator ()),
            in);
        }
      }

      void
      process_container_value (semantics::type& t,
                               semantics::names* hint,
                               semantics::data_member& m,
                               string const& prefix,
                               bool obj_ptr)
      {
        if (composite_wrapper (t))
          return;

        semantics::names* wh (0);
        semantics::type* wt (wrapper (t, wh));

        string type;
        semantics::type& ct (utype (m));

        // Custom mapping can come from these places (listed in the order
        // of priority): member, container type, value type. To complicate
        // things a bit, for object references, it can also come from the
        // member and value type of the id member.
        //
        if (m.count (prefix + "-type"))
          type = m.get<string> (prefix + "-type");

        if (type.empty () && ct.count (prefix + "-type"))
          type = ct.get<string> (prefix + "-type");

        semantics::class_* c;
        if (obj_ptr && (c = object_pointer (t)))
        {
          // This is an object pointer. The column type is the pointed-to
          // object id type.
          //
          semantics::data_member& id (*id_member (*c)->back ());

          semantics::names* idhint;
          semantics::type& idt (utype (id, idhint));

          // Nothing to do if this is a composite value type.
          //
          if (composite_wrapper (idt))
            return;

          semantics::type* wt (0);
          semantics::names* whint (0);
          if ((wt = wrapper (idt, whint)))
            wt = &utype (*wt, whint);

          if (type.empty () && id.count ("id-type"))
            type = id.get<string> ("id-type");

          if (type.empty () && id.count ("type"))
            type = id.get<string> ("type");

          // The rest of the code is identical to the else block except here
          // we have to check for "id-type" before checking for "type".
          //

          if (type.empty () && idt.count ("id-type"))
            type = idt.get<string> ("id-type");

          if (type.empty () && wt != 0 && wt->count ("id-type"))
            type = wt->get<string> ("id-type");

          if (type.empty () && idt.count ("type"))
            type = idt.get<string> ("type");

          if (type.empty () && wt != 0 && wt->count ("type"))
            type = wt->get<string> ("type");

          if (type.empty ())
            type = database_type (idt, idhint, true);

          if (type.empty () && wt != 0)
            type = database_type (*wt, whint, true);
        }
        else
        {
          if (type.empty () && t.count ("type"))
            type = t.get<string> ("type");

          if (type.empty () && wt != 0 && wt->count ("type"))
            type = wt->get<string> ("type");

          bool null (false);
          if (type.empty ())
            type = database_type (t, hint, false, &null);

          if (type.empty () && wt != 0)
            type = database_type (*wt, wh, false, &null);

          // Allow NULL if requested by the default mapping.
          //
          if (null && !m.count (prefix + "-not-null"))
            m.set (prefix + "-null", true);
        }

        if (!type.empty ())
        {
          m.set (prefix + "-column-type", type);
          m.set (prefix + "-column-id-type", type);
          return;
        }

        // We do not support nested containers so skip that test.
        //

        // If it is none of the above then we have an error.
        //
        string fq_type (t.fq_anonymous () ? "<anonymous>" : t.fq_name ());

        os << m.file () << ":" << m.line () << ":" << m.column () << ":"
           << " error: unable to map C++ type '" << fq_type << "' used in "
           << "data member '" << m.name () << "' to a " << db.name ()
           << " database type" << endl;

        os << m.file () << ":" << m.line () << ":" << m.column () << ":"
           << " info: use '#pragma db " << prefix << "_type' to specify the "
           << "database type" << endl;

        throw operation_failed ();
      }

      void
      process_container (semantics::data_member& m, semantics::type& t)
      {
        container_kind_type ck (t.get<container_kind_type> ("container-kind"));

        semantics::names* vh (0);
        semantics::names* ih (0);
        semantics::names* kh (0);

        semantics::type* vt (&utype (m, vh, "value"));
        semantics::type* it (ck == ck_ordered ? &utype (m, ih, "index") : 0);
        semantics::type* kt (ck == ck_map || ck == ck_multimap
                             ? &utype (m, kh, "key")
                             : 0);

        // Process member data.
        //
        m.set ("id-column-type", &id_column_type);

        process_container_value (*vt, vh, m, "value", true);

        if (it != 0)
          process_container_value (*it, ih, m, "index", false);

        if (kt != 0)
          process_container_value (*kt, kh, m, "key", true);
      }
    };

    //
    //
    struct view_data_member: traversal::data_member, context
    {
      view_data_member (semantics::class_& c)
          : view_ (c),
            query_ (c.get<view_query> ("query")),
            amap_ (c.get<view_alias_map> ("alias-map")),
            omap_ (c.get<view_object_map> ("object-map"))
      {
      }

      struct assoc_member
      {
        semantics::data_member* m;
        view_object* vo;
      };

      typedef vector<assoc_member> assoc_members;

      virtual void
      traverse (semantics::data_member& m)
      {
        using semantics::data_member;

        if (transient (m))
          return;

        semantics::type& t (utype (m));

        // Object pointers are associated with objects.
        //
        if (object_pointer (t))
          return;

        data_member* src_m (0); // Source member.

        // Resolve member references in column expressions.
        //
        if (m.count ("column"))
        {
          // Column literal.
          //
          if (query_.kind != view_query::condition)
          {
            warn (m.get<location_t> ("column-location"))
              << "db pragma column ignored in a view with "
              << (query_.kind == view_query::runtime ? "runtime" : "complete")
              << " query" << endl;
          }

          return;
        }
        else if (m.count ("column-expr"))
        {
          column_expr& e (m.get<column_expr> ("column-expr"));

          if (query_.kind != view_query::condition)
          {
            warn (e.loc)
              << "db pragma column ignored in a view with "
              << (query_.kind == view_query::runtime ? "runtime" : "complete")
              << " query" << endl;
            return;
          }

          for (column_expr::iterator i (e.begin ()); i != e.end (); ++i)
          {
            // This code is quite similar to translate_expression in the
            // source generator.
            //
            try
            {
              using semantics::scope;
              using semantics::class_;

              if (i->kind != column_expr_part::reference)
                continue;

              lex_.start (i->value);

              string tl;
              tree tn;
              cpp_ttype tt (lex_.next (tl, &tn));

              data_member* m (0);
              view_object* vo (0);

              // Check if this is an alias.
              //
              if (tt == CPP_NAME)
              {
                view_alias_map::iterator j (amap_.find (tl));

                if (j != amap_.end ())
                {
                  vo = j->second;

                  // Skip '::'.
                  //
                  if (lex_.next (tl, &tn) != CPP_SCOPE)
                  {
                    error (i->loc) << "member name expected after an alias " <<
                      "in db pragma column" << endl;
                    throw operation_failed ();
                  }

                  if (lex_.next (tl, &tn) != CPP_NAME)
                    throw lookup::invalid_name ();

                  m = &vo->obj->lookup<data_member> (
                    tl, scope::include_hidden);

                  tt = lex_.next (tl, &tn);
                }
              }

              // If it is not an alias, do the normal lookup.
              //
              if (vo == 0)
              {
                // Also get the object type. We need to do it so that
                // we can get the correct (derived) table name (the
                // member itself can come from a base class).
                //
                scope* s;
                string name;
                cpp_ttype ptt; // Not used.
                m = &lookup::resolve_scoped_name<data_member> (
                  lex_, tt, tl, tn, ptt,
                  dynamic_cast<scope&> (*unit.find (i->scope)),
                  name,
                  false,
                  &s);

                view_object_map::iterator j (
                  omap_.find (dynamic_cast<class_*> (s)));

                if (j == omap_.end ())
                {
                  error (i->loc) << "name '" << name << "' in db pragma " <<
                    "column does not refer to a data member of a " <<
                    "persistent class that is used in this view" << endl;
                  throw operation_failed ();
                }

                vo = j->second;
              }

              i->member_path.push_back (m);

              // Figure out the table name/alias for this member.
              //
              if (class_* root = polymorphic (*vo->obj))
              {
                // If the object is polymorphic, then figure out which of the
                // bases this member comes from and use the corresponding
                // table.
                //
                class_* c (&static_cast<class_&> (m->scope ()));

                // If this member's class is not polymorphic (root uses reuse
                // inheritance), then use the root table.
                //
                if (!polymorphic (*c))
                  c = root;

                // In a polymorphic hierarchy we have several tables and the
                // provided alias is used as a prefix together with the table
                // name to form the actual alias.
                //
                qname const& t (table_name (*c));

                if (vo->alias.empty ())
                  i->table = t;
                else
                  i->table = qname (vo->alias + "_" + t.uname ());
              }
              else
                i->table = vo->alias.empty ()
                  ? table_name (*vo->obj)
                  : qname (vo->alias);

              // Finally, resolve nested members if any.
              //
              for (; tt == CPP_DOT; tt = lex_.next (tl, &tn))
              {
                lex_.next (tl, &tn); // Get CPP_NAME.

                // Check that the outer member is composite and also
                // unwrap it while at it.
                //
                class_* comp (composite_wrapper (utype (*m)));
                if (comp == 0)
                {
                  error (i->loc) << "data member '" << m->name () << "' " <<
                    "specified in db pragma column is not composite" << endl;
                  throw operation_failed ();
                }

                m = &comp->lookup<data_member> (tl, class_::include_hidden);
                i->member_path.push_back (m);
              }

              // If the expression is just this reference, then we have
              // a source member.
              //
              if (e.size () == 1)
                src_m = m;
            }
            catch (lookup::invalid_name const&)
            {
              error (i->loc) << "invalid name in db pragma column" << endl;
              throw operation_failed ();
            }
            catch (semantics::unresolved const& e)
            {
              if (e.type_mismatch)
                error (i->loc) << "name '" << e.name << "' in db pragma " <<
                  "column does not refer to a data member" << endl;
              else
                error (i->loc) << "unable to resolve data member '" <<
                  e.name << "' specified with db pragma column" << endl;

              throw operation_failed ();
            }
            catch (semantics::ambiguous const& e)
            {
              error (i->loc) << "data member name '" << e.first.name () <<
                "' specified with db pragma column is ambiguous" << endl;

              info (e.first.named ().location ()) << "could resolve to " <<
                "this data member" << endl;

              info (e.second.named ().location ()) << "or could resolve " <<
                "to this data member" << endl;

              throw operation_failed ();
            }
          }

          // Check that the source member is not transient or inverse. Also
          // check that the C++ types are the same (sans cvr-qualification
          // and wrapping) and issue a warning if they differ. In rare cases
          // where this is not a mistake, the user can use a phony expression
          // (e.g., "" + person:name) to disable the warning. Note that in
          // this case there will be no type pragma copying, which is probably
          // ok seeing that the C++ types are different.
          //
          //
          if (src_m != 0)
          {
            string reason;

            if (transient (*src_m))
              reason = "transient";
            else if (inverse (*src_m))
              reason = "inverse";

            if (!reason.empty ())
            {
              error (e.loc)
                << "object data member '" << src_m->name () << "' specified "
                << "in db pragma column is " << reason << endl;
              throw operation_failed ();
            }

            if (!member_resolver::check_types (utype (*src_m), utype (m)))
            {
              warn (e.loc)
                << "object data member '" << src_m->name () << "' specified "
                << "in db pragma column has a different type compared to the "
                << "view data member" << endl;

              info (src_m->file (), src_m->line (), src_m->column ())
                << "object data member is defined here" << endl;

              info (m.file (), m.line (), m.column ())
                << "view data member is defined here" << endl;
            }
          }
        }
        // This member has no column information. If we are generating our
        // own query, try to find a member with the same (or similar) name
        // in one of the associated objects.
        //
        else if (query_.kind == view_query::condition)
        {
          view_objects& objs (view_.get<view_objects> ("objects"));

          assoc_members exact_members, pub_members;
          member_resolver resolver (exact_members, pub_members, m);

          for (view_objects::iterator i (objs.begin ()); i != objs.end (); ++i)
          {
            if (i->kind == view_object::object)
              resolver.traverse (*i);
          }

          assoc_members& members (
            !exact_members.empty () ? exact_members : pub_members);

          // Issue diagnostics if we didn't find any or found more
          // than one.
          //
          if (members.empty ())
          {
            error (m.file (), m.line (), m.column ())
              << "unable to find a corresponding data member for '"
              << m.name () << "' in any of the associated objects" << endl;

            info (m.file (), m.line (), m.column ())
              << "use db pragma column to specify the corresponding data "
              << "member or column name" << endl;

            throw operation_failed ();
          }
          else if (members.size () > 1)
          {
            error (m.file (), m.line (), m.column ())
              << "corresponding data member for '" << m.name () << "' is "
              << "ambiguous" << endl;

            info (m.file (), m.line (), m.column ())
              << "candidates are:" << endl;

            for (assoc_members::const_iterator i (members.begin ());
                 i != members.end ();
                 ++i)
            {
              info (i->m->file (), i->m->line (), i->m->column ())
                << "  '" << i->m->name () << "' in object '"
                << i->vo->name () << "'" << endl;
            }

            info (m.file (), m.line (), m.column ())
              << "use db pragma column to resolve this ambiguity" << endl;

            throw operation_failed ();
          }

          // Synthesize the column expression for this member.
          //
          assoc_member const& am (members.back ());

          column_expr& e (m.set ("column-expr", column_expr ()));
          e.push_back (column_expr_part ());
          column_expr_part& ep (e.back ());

          ep.kind = column_expr_part::reference;


          // If this object is polymorphic, then figure out which of the
          // bases this member comes from and use the corresponding table.
          //
          using semantics::class_;

          if (class_* root = polymorphic (*am.vo->obj))
          {
            class_* c (&static_cast<class_&> (am.m->scope ()));

            // If this member's class is not polymorphic (root uses reuse
            // inheritance), then use the root table.
            //
            if (!polymorphic (*c))
              c = root;

            // In a polymorphic hierarchy we have several tables and the
            // provided alias is used as a prefix together with the table
            // name to form the actual alias.
            //
            qname const& t (table_name (*c));

            if (am.vo->alias.empty ())
              ep.table = t;
            else
              ep.table = qname (am.vo->alias + "_" + t.uname ());
          }
          else
            ep.table = am.vo->alias.empty ()
              ? table_name (*am.vo->obj)
              : qname (am.vo->alias);

          ep.member_path.push_back (am.m);

          src_m = am.m;
        }

        // If we have the source member and don't have the type pragma of
        // our own, but the source member does, then copy the columnt type
        // over. In case the source member is a pointer, also check the id
        // member.
        //
        if (src_m != 0 && !m.count ("type"))
        {
          if (src_m->count ("type"))
            m.set ("column-type", src_m->get<string> ("column-type"));
          else if (semantics::class_* c = object_pointer (utype (*src_m)))
          {
            semantics::data_member& id (*id_member (*c)->back ());

            if (id.count ("type"))
              m.set ("column-type", id.get<string> ("column-type"));
          }
        }

        // Check the return statements above if you add any extra logic
        // here.
      }

      struct member_resolver: traversal::class_
      {
        member_resolver (assoc_members& members,
                         assoc_members& pub_members,
                         semantics::data_member& m)
            : member_ (members, pub_members, m)
        {
          *this >> names_ >> member_;
          *this >> inherits_ >> *this;
        }

        void
        traverse (view_object& vo)
        {
          member_.vo_ = &vo;

          // First look for an exact match.
          //
          {
            member_.exact_ = true;
            member_.found_ = false;
            traverse (*vo.obj);
          }

          // If we didn't find an exact match, then look for a public
          // name match.
          //
          if (!member_.found_)
          {
            member_.exact_ = false;
            traverse (*vo.obj);
          }
        }

        virtual void
        traverse (type& c)
        {
          if (!object (c))
            return; // Ignore transient bases.

          names (c);

          // If we already found a match in one of the derived classes,
          // don't go into bases to get the standard "hiding" behavior.
          //
          if (!member_.found_)
            inherits (c);
        }

      public:
        static bool
        check_types (semantics::type& ot, semantics::type& vt)
        {
          using semantics::type;

          // Require that the types be the same sans the wrapping and
          // cvr-qualification. If the object member type is a pointer,
          // use the id type of the pointed-to object.
          //
          type* t1;

          if (semantics::class_* c = object_pointer (ot))
            t1 = &utype (*id_member (*c));
          else
            t1 = &ot;

          type* t2 (&vt);

          if (type* wt1 = context::wrapper (*t1))
            t1 = &utype (*wt1);

          if (type* wt2 = context::wrapper (*t2))
            t2 = &utype (*wt2);

          if (t1 != t2)
            return false;

          return true;
        }

      private:
        struct data_member: traversal::data_member
        {
          data_member (assoc_members& members,
                       assoc_members& pub_members,
                       semantics::data_member& m)
              : members_ (members),
                pub_members_ (pub_members),
                name_ (m.name ()),
                pub_name_ (context::current ().public_name (m)),
                type_ (utype (m))
          {
          }

          virtual void
          traverse (type& m)
          {
            if (exact_)
            {
              if (name_ == m.name () && check (m))
              {
                assoc_member am;
                am.m = &m;
                am.vo = vo_;
                members_.push_back (am);
                found_ = true;
              }
            }
            else
            {
              if (pub_name_ == context::current ().public_name (m) &&
                  check (m))
              {
                assoc_member am;
                am.m = &m;
                am.vo = vo_;
                pub_members_.push_back (am);
                found_ = true;
              }
            }
          }

          bool
          check (semantics::data_member& m)
          {
            // Make sure that the found node can possibly match.
            //
            if (context::transient (m) ||
                context::inverse (m) ||
                m.count ("polymorphic-ref"))
              return false;

            return check_types (utype (m), type_);
          }

          assoc_members& members_;
          assoc_members& pub_members_;

          string name_;
          string pub_name_;
          semantics::type& type_;

          view_object* vo_;
          bool exact_;
          bool found_;
        };

        traversal::names names_;
        data_member member_;
        traversal::inherits inherits_;
      };

    private:
      semantics::class_& view_;
      view_query& query_;
      view_alias_map& amap_;
      view_object_map& omap_;
      cxx_string_lexer lex_;
    };

    struct class_: traversal::class_, context
    {
      class_ ()
          : typedefs_ (true)
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;

        member_names_ >> member_;
      }

      virtual void
      traverse (type& c)
      {
        class_kind_type k (class_kind (c));

        if (k == class_other)
          return;

        names (c); // Process nested classes.
        names (c, member_names_);

        if (k == class_object)
          traverse_object (c);
        else if (k == class_view)
          traverse_view (c);
      }

      //
      // Object.
      //

      virtual void
      traverse_object (type& c)
      {
        // Remove the bulk pragma if this database doesn't support bulk
        // operations.
        //
        if (c.count ("bulk") && !generate_bulk)
          c.remove ("bulk");

        // Process indexes. Here we need to do two things: resolve member
        // names to member paths and assign names to unnamed indexes. We
        // are also going to handle the special container indexes.
        //
        indexes& ins (c.count ("index")
                      ? c.get<indexes> ("index")
                      : c.set ("index", indexes ()));

        for (indexes::iterator i (ins.begin ()); i != ins.end ();)
        {
          index& in (*i);

          // This should never happen since a db index pragma without
          // the member specifier will be treated as a member pragma.
          //
          assert (!in.members.empty ());

          // First resolve member names.
          //
          index::members_type::iterator j (in.members.begin ());
          for (; j != in.members.end (); ++j)
          {
            index::member& im (*j);

            if (!im.path.empty ())
              continue; // Already resolved.

            im.path = resolve_data_members (c, im.name, im.loc, lex_);

            if (container (*im.path.back ()))
              break;
          }

          // Add the table prefix if this database has global index names.
          //
          if (!in.name.empty () && global_index)
            in.name = table_name_prefix (class_scope (c)) + in.name;

          // Handle container indexes.
          //
          if (j != in.members.end ())
          {
            // Do some sanity checks.
            //
            if (in.members.size () != 1)
            {
              error (in.loc) << "multiple data members specified for a "
                             << "container index" << endl;
              throw operation_failed ();
            }

            string tl;
            if (lex_.next (tl) != CPP_DOT || lex_.next (tl) != CPP_NAME ||
                (tl != "id" && tl != "index"))
            {
              error (j->loc) << ".id or .index special member expected in a "
                             << "container index" << endl;
              throw operation_failed ();
            }

            string n (tl);

            if (lex_.next (tl) != CPP_EOF)
            {
              error (j->loc) << "unexpected text after ." << n << " in "
                             << "db pragma member" << endl;
              throw operation_failed ();
            }

            // Move this index to the container member.
            //
            j->path.back ()->set (n + "-index", *i);
            i = ins.erase (i);
            continue;
          }

          // Now assign the name if the index is unnamed. We have to
          // add table name as a prefix here since there is not way
          // to distinguish between user-assigned and auto-derived
          // names in the model.
          //
          if (in.name.empty ())
          {
            // Make sure there is only one member.
            //
            if (in.members.size () > 1)
            {
              error (in.loc) << "unnamed index with more than one data "
                             << "member" << endl;
              throw operation_failed ();
            }

            // Generally, we want the index name to be based on the column
            // name. This is straightforward for single-column members. In
            // case of a composite member, we will need to use the column
            // prefix which is based on the data member name, unless
            // overridden by the user. In the latter case the prefix can
            // be empty, in which case we will just fall back on the
            // member's public name.
            //
            string n (column_prefix (in.members.front ().path, true).prefix);

            if (n.empty ())
              n = public_name_db (*in.members.front ().path.back ());
            else if (n[n.size () - 1] == '_')
              n.resize (n.size () - 1); // Remove trailing underscore.

            in.name = index_name (table_name (c), n);
          }

          ++i;
        }
      }

      //
      // View.
      //

      struct relationship
      {
        semantics::data_member* member;
        string name;
        view_object* pointer;
        view_object* pointee;
      };

      typedef vector<relationship> relationships;

      virtual void
      traverse_view (type& c)
      {
        bool has_q (c.count ("query"));
        bool has_o (c.count ("objects"));

        // Determine the kind of query template we've got.
        //
        view_query& vq (has_q
                        ? c.get<view_query> ("query")
                        : c.set ("query", view_query ()));
        if (has_q)
        {
          if (!vq.literal.empty ())
          {
            string q (upcase (vq.literal));

            //@@ We need to recognize database-specific list of prefixes. For
            //   example, PG has WITH. Alternatively (or in addition) we could
            //   do the same comment trick (e.g., /*SELECT*/ to treat it as a
            //   SELECT-like queiry).
            //
            if (q.compare (0, 7, "SELECT ") == 0)
              vq.kind = view_query::complete_select;
            else if (q.compare (0, 5, "EXEC ") == 0 ||
                     q.compare (0, 5, "CALL ") == 0 ||
                     q.compare (0, 8, "EXECUTE ") == 0)
              vq.kind = view_query::complete_execute;
            //
            // Hint for databases that use SELECT for stored procedure
            // calls (e.g., PostgreSQL).
            //
            else if (q.compare (0, 8, "/*CALL*/") == 0)
            {
              vq.literal = string (vq.literal, q[8] == ' ' ? 9 : 8);
              vq.kind = view_query::complete_execute;
            }
            else
              vq.kind = view_query::condition;
          }
          else if (!vq.expr.empty ())
          {
            // If the first token in the expression is a string and
            // it starts with "SELECT " or is equal to "SELECT" or
            // one of the stored procedure call keywords, then we
            // have a complete query.
            //
            if (vq.expr.front ().type == CPP_STRING)
            {
              string q (upcase (vq.expr.front ().literal));

              if (q.compare (0, 7, "SELECT ") == 0 || q == "SELECT")
                vq.kind = view_query::complete_select;
              else if (q.compare (0, 5, "EXEC ") == 0 || q == "EXEC" ||
                       q.compare (0, 5, "CALL ") == 0 || q == "CALL" ||
                       q.compare (0, 8, "EXECUTE ") == 0 || q == "EXECUTE")
                vq.kind = view_query::complete_execute;
              else if (q.compare (0, 8, "/*CALL*/") == 0)
              {
                vq.expr.front ().literal =
                  string (vq.expr.front ().literal, q[8] == ' ' ? 9 : 8);
                vq.kind = view_query::complete_execute;
              }
              else
                vq.kind = view_query::condition;
            }
            else
              vq.kind = view_query::condition;
          }
          else
            vq.kind = (vq.distinct || vq.for_update)
              ? view_query::condition // The query(distinct) case.
              : view_query::runtime;
        }
        else
          vq.kind = has_o ? view_query::condition : view_query::runtime;

        if ((vq.distinct || vq.for_update) && vq.kind != view_query::condition)
        {
          error (vq.loc)
            << "result modifier specified for "
            << (vq.kind == view_query::runtime ? "runtime" : "native")
            << " query" << endl;

          throw operation_failed ();
        }

        // We cannot have an incomplete query if there are not objects
        // to derive the rest from.
        //
        if (vq.kind == view_query::condition && !has_o)
        {
          error (c.file (), c.line (), c.column ())
            << "view '" << class_fq_name (c) << "' has an incomplete query "
            << "template and no associated objects" << endl;

          info (c.file (), c.line (), c.column ())
            << "use db pragma query to provide a complete query template"
            << endl;

          info (c.file (), c.line (), c.column ())
            << "or use db pragma object to associate one or more objects "
            << "with the view"
            << endl;

          throw operation_failed ();
        }

        // Process join conditions.
        //
        if (has_o)
        {
          view_objects& objs (c.get<view_objects> ("objects"));

          for (view_objects::iterator i (objs.begin ()); i != objs.end (); ++i)
          {
            if (i == objs.begin () && i->join != view_object::left)
            {
              error (i->loc)
                << "no join type can be specified for the first associated "
                << (i->kind == view_object::object ? "object" : "table")
                << endl;
              throw operation_failed ();
            }

            if (i->kind != view_object::object)
            {
              // Make sure we have join conditions for tables unless it
              // is the first entry.
              //
              if (i != objs.begin () && i->cond.empty ())
              {
                error (i->loc)
                  << "missing join condition in db pragma table" << endl;

                throw operation_failed ();
              }

              continue;
            }

            // If we have to generate the query and there was no JOIN
            // condition specified by the user, try to come up with one
            // automatically based on object relationships. CROSS JOIN
            // has no condition.
            //
            if (vq.kind == view_query::condition &&
                i->cond.empty () &&
                i != objs.begin () &&
                i->join != view_object::cross)
            {
              relationships rs;

              // Check objects specified prior to this one for any
              // relationships. We don't examine objects that were
              // specified after this one because that would require
              // rearranging the JOIN order.
              //
              for (view_objects::iterator j (objs.begin ()); j != i; ++j)
              {
                if (j->kind != view_object::object)
                  continue; // Skip tables.

                // First see if any of the objects that were specified
                // prior to this object point to it.
                //
                {
                  relationship_resolver r (rs, *i, true);
                  r.traverse (*j);
                }

                // Now see if this object points to any of the objects
                // specified prior to it.
                //
                {
                  relationship_resolver r (rs, *j, false);
                  r.traverse (*i);
                }
              }

              // Issue diagnostics if we didn't find any or found more
              // than one.
              //
              if (rs.empty ())
              {
                error (i->loc)
                  << "unable to find an object relationship involving "
                  << "object '" << i->name () << "' and any of the previously "
                  << "associated objects" << endl;

                info (i->loc)
                  << "use the join condition clause in db pragma object "
                  << "to specify a custom join condition" << endl;

                throw operation_failed ();
              }
              else if (rs.size () > 1)
              {
                error (i->loc)
                  << "object relationship for object '" << i->name () <<  "' "
                  << "is ambiguous" << endl;

                info (i->loc)
                  << "candidates are:" << endl;

                for (relationships::const_iterator j (rs.begin ());
                     j != rs.end ();
                     ++j)
                {
                  semantics::data_member& m (*j->member);

                  info (m.file (), m.line (), m.column ())
                    << "  '" << j->name << "' "
                    << "in object '" << j->pointer->name () << "' "
                    << "pointing to '" << j->pointee->name () << "'"
                    << endl;
                }

                info (i->loc)
                  << "use the join condition clause in db pragma object "
                  << "to resolve this ambiguity" << endl;

                throw operation_failed ();
              }

              // Synthesize the condition.
              //
              relationship const& r (rs.back ());

              string name (r.pointer->alias.empty ()
                           ? class_fq_name (*r.pointer->obj)
                           : r.pointer->alias);
              name += "::";
              name += r.name;

              lex_.start (name);

              string t;
              for (cpp_ttype tt (lex_.next (t));
                   tt != CPP_EOF;
                   tt = lex_.next (t))
              {
                i->cond.push_back (cxx_token (lex_.location (), tt, t));
              }
            }
          }
        }

        // Handle forced versioning. When versioning is forced, ignore
        // it for native views.
        //
        if (force_versioned && vq.kind == view_query::condition)
          c.set ("versioned", true);

        // Handle data members.
        //
        {
          view_data_member t (c);
          traversal::names n (t);
          names (c, n);
        }
      }

      struct relationship_resolver: object_members_base
      {
        relationship_resolver (relationships& rs,
                               view_object& pointee,
                               bool forward)
            // Look in polymorphic bases only for previously-associated
            // objects since backward pointers from bases will result in
            // the pathological case (we will have to join the base table
            // first, which means we will get both bases and derived objects
            // instead of just derived).
            //
            : object_members_base (false, false, true, forward),
              relationships_ (rs),
              // Ignore self-references if we are looking for backward
              // pointers since they were already added to the list in
              // the previous pass.
              //
              self_pointer_ (forward),
              pointer_ (0),
              pointee_ (pointee)
        {
        }

        void
        traverse (view_object& pointer)
        {
          pointer_ = &pointer;
          object_members_base::traverse (*pointer.obj);
        }

        using object_members_base::traverse; // Unhide.

        virtual void
        traverse_pointer (semantics::data_member& m, semantics::class_& c)
        {
          // Ignore polymorphic id references.
          //
          if (m.count ("polymorphic-ref"))
            return;

          // Ignore inverse sides of the same relationship to avoid
          // phony conflicts caused by the direct side that will end
          // up in the relationship list as well. Unless the inverse
          // member is in the polymorphic base in which case we will
          // miss it since we don't examine inside poly bases on the
          // backwards scan (see above).
          //
          if (data_member_path* imp = inverse (m))
          {
            if (&imp->front ()->scope () == &c) // Direct member.
              return;
          }

          // Ignore self-pointers if requested.
          //
          if (!self_pointer_ && pointer_->obj == &c)
            return;

          if (pointee_.obj == &c)
          {
            relationships_.push_back (relationship ());
            relationships_.back ().member = &m;
            relationships_.back ().name = member_prefix_ + m.name ();
            relationships_.back ().pointer = pointer_;
            relationships_.back ().pointee = &pointee_;
          }
        }

        virtual void
        traverse_container (semantics::data_member& m, semantics::type&)
        {
          if (semantics::class_* c =
              object_pointer (context::container_vt (m)))
          {
            if (inverse (m, "value"))
              return;

            // Ignore self-pointers if requested.
            //
            if (!self_pointer_ && pointer_->obj == c)
              return;

            if (pointee_.obj == c)
            {
              relationships_.push_back (relationship ());
              relationships_.back ().member = &m;
              relationships_.back ().name = member_prefix_ + m.name ();
              relationships_.back ().pointer = pointer_;
              relationships_.back ().pointee = &pointee_;
            }
          }
        }

      private:
        relationships& relationships_;
        bool self_pointer_;
        view_object* pointer_;
        view_object& pointee_;
      };

    private:
      cxx_string_lexer lex_;

      traversal::defines defines_;
      typedefs typedefs_;

      data_member member_;
      traversal::names member_names_;
    };
  }

  void
  process ()
  {
    context ctx;

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

    unit.dispatch (ctx.unit);
  }
}
