// file      : odb/relational/model.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_MODEL_HXX
#define ODB_RELATIONAL_MODEL_HXX

#include <map>
#include <set>
#include <cassert>
#include <sstream>

#include <odb/semantics/relational.hxx>

#include <odb/relational/common.hxx>
#include <odb/relational/context.hxx>

namespace relational
{
  namespace model
  {
    typedef std::set<qname> tables;
    typedef std::map<qname, semantics::node*> deleted_table_map;
    typedef std::map<uname, semantics::data_member*> deleted_column_map;

    struct object_columns: object_columns_base, virtual context
    {
      typedef object_columns base;

      object_columns (sema_rel::model& model,
                      sema_rel::table& table,
                      bool object)
          : model_ (model),
            table_ (table),
            object_ (object),
            pkey_ (0),
            id_override_ (false)
      {
      }

      virtual void
      traverse_object (semantics::class_& c)
      {
        if (context::top_object != &c)
        {
          // We are in one of the bases. Set the id_prefix to its
          // (unqualified) name.
          //
          string t (id_prefix_);
          id_prefix_ = class_name (c) + "::";
          object_columns_base::traverse_object (c);
          id_prefix_ = t;
        }
        else
          object_columns_base::traverse_object (c);
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        string t (id_prefix_);

        if (m != 0)
          // Member of a composite type. Add the data member to id_prefix.
          //
          if (!id_override_)
            id_prefix_ += m->name () + ".";
          else
            id_override_ = false;
        else
          // Composite base. Add its unqualified name to id_prefix.
          //
          id_prefix_ += class_name (c) + "::";

        object_columns_base::traverse_composite (m, c);

        id_prefix_ = t;
      }

      virtual void
      traverse (semantics::data_member& m,
                semantics::type& t,
                string const& kp,
                string const& dn,
                semantics::class_* to = 0)
      {
        // This overrides the member name for a composite container value
        // or key.
        //
        if (!kp.empty ())
        {
          semantics::class_* c (object_pointer (t));
          if (composite_wrapper (c == 0 ? t : utype (*id_member (*c))))
          {
            id_prefix_ = kp + ".";
            id_override_ = true;
          }
        }

        object_columns_base::traverse (m, t, kp, dn, to);
      }

      using object_columns_base::traverse;

      virtual bool
      traverse_column (semantics::data_member& m, string const& name, bool)
      {
        if (semantics::data_member* m = deleted_member (member_path_))
        {
          table_.get<deleted_column_map> ("deleted-map")[name] = m;
          return false;
        }

        string col_id (id_prefix_ +
                       (key_prefix_.empty () ? m.name () : key_prefix_));

        sema_rel::column& c (
          model_.new_node<sema_rel::column> (col_id, type (m), null (m)));
        c.set ("cxx-location", m.location ());
        c.set ("member-path", member_path_);
        model_.new_edge<sema_rel::unames> (table_, c, name);

        // An id member cannot have a default value.
        //
        if (!object_columns_base::id ())
        {
          string const& d (default_ (m));

          if (!d.empty ())
            c.default_ (d);
        }

        // If we have options, add them.
        //
        string const& o (column_options (m, key_prefix_));

        if (!o.empty ())
          c.options (o);

        constraints (m, name, col_id, c);
        return true;
      }

      virtual string
      type (semantics::data_member&)
      {
        return object_columns_base::column_type ();
      }

      virtual bool
      null (semantics::data_member&)
      {
        return !object_columns_base::id () && object_columns_base::null ();
      }

      virtual string
      default_null (semantics::data_member&)
      {
        return "NULL";
      }

      virtual string
      default_bool (semantics::data_member&, bool v)
      {
        // Most databases do not support boolean literals. Those that
        // do should override this.
        //
        return (v ? "1" : "0");
      }

      virtual string
      default_integer (semantics::data_member&, unsigned long long v, bool neg)
      {
        std::ostringstream ostr;
        ostr << (neg ? "-" : "") << v;
        return ostr.str ();
      }

      virtual string
      default_float (semantics::data_member&, double v)
      {
        std::ostringstream ostr;
        ostr << v;
        return ostr.str ();
      }

      virtual string
      default_string (semantics::data_member&, string const& v)
      {
        return quote_string (v);
      }

      virtual string
      default_enum (semantics::data_member&,
                    tree /*enumerator*/,
                    string const& /*name*/)
      {
        // Has to be implemented by the database-specific override.
        //
        assert (false);
        return string ();
      }

      virtual void
      primary_key (sema_rel::primary_key&)
      {
      }

      virtual void
      constraints (semantics::data_member& m,
                   string const& /* name */,
                   string const& /* id */,
                   sema_rel::column& c)
      {
        if (object_)
        {
          if (semantics::data_member* idm = id ())
          {
            if (pkey_ == 0)
            {
              pkey_ = &model_.new_node<sema_rel::primary_key> (
                m.count ("auto"));
              pkey_->set ("cxx-location", idm->location ());

              // In most databases the primary key constraint can be
              // manipulated without an explicit name. So we use the special
              // empty name for primary keys in order not to clash with
              // columns and other constraints. If the target database does
              // not support unnamed primary key manipulation, then the
              // database-specific code will have to come up with a suitable
              // name.
              //
              model_.new_edge<sema_rel::unames> (table_, *pkey_, "");
              primary_key (*pkey_);
            }

            model_.new_edge<sema_rel::contains> (*pkey_, c);
          }
        }
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_& c)
      {
        // Ignore inverse object pointers.
        //
        if (inverse (m, key_prefix_))
          return;

        if (deleted (member_path_))
        {
          // Still traverse it as columns so that we can populate the
          // deleted map.
          //
          object_columns_base::traverse_pointer (m, c);
          return;
        }

        // Get the position of the last column.
        //
        sema_rel::table::names_iterator i (table_.names_end ());

        while (i != table_.names_begin ())
        {
          --i;
          if (i->nameable ().is_a<sema_rel::column> ())
            break;
        }

        // Traverse the object pointer as columns.
        //
        object_columns_base::traverse_pointer (m, c);

        // Get to the first column that we have added.
        //
        if (i != table_.names_end ())
          ++i; // Next column.
        else
          i = table_.names_begin ();

        foreign_key (m, c, i);
      }

      virtual void
      traverse_points_to (semantics::data_member& m, semantics::class_& c)
      {
        if (deleted (member_path_))
        {
          // Still traverse it as columns so that we can populate the
          // deleted map.
          //
          object_columns_base::traverse_points_to (m, c);
          return;
        }

        // Get the position of the last column.
        //
        sema_rel::table::names_iterator i (table_.names_end ());

        while (i != table_.names_begin ())
        {
          --i;
          if (i->nameable ().is_a<sema_rel::column> ())
            break;
        }

        // Traverse the data member as columns.
        //
        object_columns_base::traverse_points_to (m, c);

        // Get to the first column that we have added.
        //
        if (i != table_.names_end ())
          ++i; // Next column.
        else
          i = table_.names_begin ();

        foreign_key (m, c, i);
      }

      virtual void
      foreign_key (semantics::data_member& m,
                   semantics::class_& c,
                   sema_rel::table::names_iterator i)
      {
        using sema_rel::column;
        using sema_rel::foreign_key;

        string id (id_prefix_ +
                   (key_prefix_.empty () ? m.name () : key_prefix_));

        deferrable def (
          m.get<deferrable> ("deferrable",
                             options.fkeys_deferrable_mode ()[db]));

        foreign_key::action_type on_delete (
          m.get<foreign_key::action_type> (
            "on-delete", foreign_key::no_action));

        foreign_key& fk (
          model_.new_node<foreign_key> (id, table_name (c), def, on_delete));

        fk.set ("cxx-location", m.location ());

        bool simple;

        // Get referenced columns.
        //
        {
          data_member_path& id (*id_member (c));

          instance<object_columns_list> ocl;
          ocl->traverse (id);

          for (object_columns_list::iterator i (ocl->begin ());
               i != ocl->end (); ++i)
            fk.referenced_columns ().push_back (i->name);

          simple = (fk.referenced_columns ().size () == 1);
        }

        // Get referencing columns.
        //
        for (; i != table_.names_end (); ++i)
        {
          if (column* c = dynamic_cast<column*> (&i->nameable ()))
            model_.new_edge<sema_rel::contains> (fk, *c);
          else
            break;
        }

        // Derive the constraint name. Generally, we want it to be based
        // on the column name. This is straightforward for single-column
        // references. In case of a composite id, we will need to use the
        // column prefix which is based on the data member name, unless
        // overridden by the user. In the latter case the prefix can be
        // empty, in which case we will just fall back on the member's
        // public name.
        //
        string name;

        if (simple)
          name = fk.contains_begin ()->column ().name ();
        else
        {
          string p (column_prefix (m, key_prefix_, default_name_).prefix);

          if (p.empty ())
            p = public_name_db (m);
          else if (p[p.size () - 1] == '_')
            p.resize (p.size () - 1); // Remove trailing underscore.

          name = column_prefix_.prefix + p;
        }

        model_.new_edge<sema_rel::unames> (
          table_, fk, fkey_name (table_.name (), name));
      }

    protected:
      string
      default_ (semantics::data_member&);

    protected:
      sema_rel::model& model_;
      sema_rel::table& table_;
      bool object_;
      sema_rel::primary_key* pkey_;
      string id_prefix_;
      bool id_override_;
    };

    struct object_indexes: traversal::class_, virtual context
    {
      typedef object_indexes base;

      object_indexes (sema_rel::model& model, sema_rel::table& table)
          : model_ (model), table_ (table)
      {
        *this >> inherits_ >> *this;
      }

      object_indexes (object_indexes const& x)
          : root_context (), context (), //@@ -Wextra
            model_ (x.model_), table_ (x.table_)
      {
        *this >> inherits_ >> *this;
      }

      virtual void
      traverse (type& c)
      {
        if (!object (c)) // Ignore transient bases.
          return;

        // Polymorphic bases get their own tables.
        //
        if (!polymorphic (c))
          inherits (c);

        indexes& ins (c.get<indexes> ("index"));

        for (indexes::iterator i (ins.begin ()); i != ins.end (); ++i)
        {
          // Using index name as its id.
          //
          sema_rel::index& in (
            model_.new_node<sema_rel::index> (
              i->name, i->type, i->method, i->options));
          in.set ("cxx-location", location (i->loc));
          model_.new_edge<sema_rel::unames> (table_, in, i->name);

          for (index::members_type::iterator j (i->members.begin ());
               j != i->members.end (); ++j)
          {
            using sema_rel::column;

            index::member& im (*j);

            semantics::type* t (&utype (*im.path.back ()));

            if (semantics::class_* ptr = object_pointer (*t))
              t = &utype (*id_member (*ptr));

            if (type* comp = composite_wrapper (*t))
            {
              // Composite value. Get the list of the columns. Note that
              // the column prefix needs to contain all the components.
              //
              instance<object_columns_list> ocl (
                column_prefix (im.path, true));
              ocl->traverse (*comp);

              for (object_columns_list::iterator i (ocl->begin ());
                   i != ocl->end (); ++i)
              {
                column* c (table_.find<column> (i->name));
                assert (c != 0);
                model_.new_edge<sema_rel::contains> (in, *c, im.options);
              }
            }
            else
            {
              // Simple value. Get the column name and look it up in the
              // table.
              //
              column* c (table_.find<column> (column_name (im.path)));
              assert (c != 0);
              model_.new_edge<sema_rel::contains> (in, *c, im.options);
            }
          }
        }
      }

    private:
      sema_rel::model& model_;
      sema_rel::table& table_;

      traversal::inherits inherits_;
    };

    struct member_create: object_members_base, virtual context
    {
      typedef member_create base;

      member_create (sema_rel::model& model)
          : object_members_base (false, true, false), model_ (model)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member&, semantics::class_&)
      {
        // We don't want to traverse composite id.
      }

      virtual void
      traverse_object (semantics::class_& c)
      {
        if (context::top_object != &c)
        {
          // We are in one of the bases. Set the id_prefix to its
          // (unqualified) name.
          //
          string t (id_prefix_);
          id_prefix_ = class_name (c) + "::";
          object_members_base::traverse_object (c);
          id_prefix_ = t;
        }
        else
        {
          // Top-level object. Set its id as a prefix.
          //
          id_prefix_ = string (class_fq_name (c), 2) + "::";
          object_members_base::traverse_object (c);
        }
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        string t (id_prefix_);

        if (m != 0)
          // Member of a composite type. Add the data member to id_prefix.
          //
          id_prefix_ += m->name () + ".";
        else
          // Composite base. Add its unqualified name to id_prefix.
          //
          id_prefix_ += class_name (c) + "::";

        object_members_base::traverse_composite (m, c);

        id_prefix_ = t;
      }

      virtual string
      table_options (semantics::data_member& m, semantics::type& ct)
      {
        return context::table_options (m, ct);
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type& ct)
      {
        using semantics::type;
        using semantics::data_member;

        using sema_rel::column;

        // Ignore inverse containers of object pointers.
        //
        if (inverse (m, "value"))
          return;

        container_kind_type ck (container_kind (ct));
        qname const& name (table_name (m, table_prefix_));

        // Ignore deleted container members.
        //
        if (semantics::data_member* m = deleted_member (member_path_))
        {
          model_.get<deleted_table_map> ("deleted-map")[name] = m;
          return;
        }

        // Add the [] decorator to distinguish this id from non-container
        // ids (we don't want to ever end up comparing, for example, an
        // object table to a container table).
        //
        string id (id_prefix_ + m.name () + "[]");

        sema_rel::table& t (model_.new_node<sema_rel::table> (id));
        t.set ("cxx-location", m.location ());
        t.set ("member-path", member_path_);
        t.set ("deleted-map", deleted_column_map ());
        model_.new_edge<sema_rel::qnames> (model_, t, name);

        t.options (table_options (m, ct));
        t.extra ()["kind"] = "container";

        // object_id
        //
        {
          bool f (false); //@@ (im)persfect forwarding.
          instance<object_columns> oc (model_, t, f);
          oc->traverse (m, container_idt (m), "id", "object_id");
        }

        // Foreign key and index for the object id. Keep this foreign
        // key first since we reply on this information to lookup the
        // corresponding object table.
        //
        {
          // Derive the name prefix. See the comment for the other foreign
          // key code above.
          //
          // Note also that id_name can be a column prefix (if id is
          // composite), in which case it can be empty. In this case
          // we just fallback on the default name.
          //
          // Finally, this is a top-level column, so there is no column
          // prefix.
          //
          string id_name (
            column_name (m, "id", "object_id", column_prefix ()));

          if (id_name.empty ())
            id_name = "object_id";

          // Foreign key.
          //
          sema_rel::foreign_key& fk (
            model_.new_node<sema_rel::foreign_key> (
              id + ".id",
              table_name (*context::top_object),
              sema_rel::deferrable::not_deferrable,
              sema_rel::foreign_key::cascade));
          fk.set ("cxx-location", m.location ());
          model_.new_edge<sema_rel::unames> (
            t, fk, fkey_name (t.name (), id_name));

          // Get referenced columns.
          //
          {
            data_member_path& id (*id_member (*context::top_object));

            instance<object_columns_list> ocl;
            ocl->traverse (id);

            for (object_columns_list::iterator i (ocl->begin ());
                 i != ocl->end (); ++i)
              fk.referenced_columns ().push_back (i->name);
          }

          // All the columns we have in this table so far are for the
          // object id. Add them to the foreign key.
          //
          for (sema_rel::table::names_iterator i (t.names_begin ());
               i != t.names_end ();
               ++i)
          {
            if (column* c = dynamic_cast<column*> (&i->nameable ()))
              model_.new_edge<sema_rel::contains> (fk, *c);
          }

          // Index. See if we have a custom index.
          //
          index* sin (m.count ("id-index") ? &m.get<index> ("id-index") : 0);
          sema_rel::index* in (0);

          if (sin != 0)
          {
            in = &model_.new_node<sema_rel::index> (
              id + ".id", sin->type, sin->method, sin->options);
            in->set ("cxx-location", location (sin->loc));
          }
          else
          {
            in = &model_.new_node<sema_rel::index> (id + ".id");
            in->set ("cxx-location", m.location ());
          }

          model_.new_edge<sema_rel::unames> (
            t,
            *in,
            sin != 0 && !sin->name.empty ()
            ? sin->name
            : index_name (name, id_name));

          // All the columns we have in this table so far are for the
          // object id. Add them to the index.
          //
          for (sema_rel::table::names_iterator i (t.names_begin ());
               i != t.names_end ();
               ++i)
          {
            if (column* c = dynamic_cast<column*> (&i->nameable ()))
              model_.new_edge<sema_rel::contains> (
                *in, *c, (sin != 0 ? sin->members.back ().options : ""));
          }
        }

        // index (simple value)
        //
        bool ordered (ck == ck_ordered && !unordered (m));
        if (ordered)
        {
          // Column.
          //
          {
            bool f (false); //@@ (im)persfect forwarding.
            instance<object_columns> oc (model_, t, f);
            oc->traverse (m, container_it (m), "index", "index");
          }

          // This is a simple value so the name cannot be empty. It is
          // also a top-level column, so there is no column prefix.
          //
          string col (column_name (m, "index", "index", column_prefix ()));

          // Index. See if we have a custom index.
          //
          index* sin (m.count ("index-index")
                      ? &m.get<index> ("index-index")
                      : 0);
          sema_rel::index* in (0);

          if (sin != 0)
          {
            in = &model_.new_node<sema_rel::index> (
              id + ".index", sin->type, sin->method, sin->options);
            in->set ("cxx-location", location (sin->loc));
          }
          else
          {
            in = &model_.new_node<sema_rel::index> (id + ".index");
            in->set ("cxx-location", m.location ());
          }

          model_.new_edge<sema_rel::unames> (
            t,
            *in,
            sin != 0 && !sin->name.empty ()
            ? sin->name
            : index_name (name, col));

          column* c (t.find<column> (col));
          assert (c != 0);

          model_.new_edge<sema_rel::contains> (
            *in,
            *c,
            (sin != 0 ? sin->members.back ().options : ""));
        }

        // key
        //
        if (ck == ck_map || ck == ck_multimap)
        {
          bool f (false); //@@ (im)persfect forwarding.
          instance<object_columns> oc (model_, t, f);
          oc->traverse (m, container_kt (m), "key", "key");
        }

        // value
        //
        {
          bool f (false); //@@ (im)persfect forwarding.
          instance<object_columns> oc (model_, t, f);
          oc->traverse (m, container_vt (m), "value", "value");
        }
      }

    protected:
      sema_rel::model& model_;
      string id_prefix_;
    };

    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      class_ (sema_rel::model& model): model_ (model) {}

      virtual string
      table_options (type& c)
      {
        return context::table_options (c);
      }

      virtual void
      traverse (type& c)
      {
        if (!options.at_once () && class_file (c) != unit.file ())
          return;

        if (!object (c))
          return;

        semantics::class_* poly (polymorphic (c));

        if (abstract (c) && poly == 0)
          return;

        qname const& name (table_name (c));

        // If the table with this name was already seen, assume the
        // user knows what they are doing and skip it.
        //
        if (tables_.count (name))
          return;

        if (deleted (c))
        {
          model_.get<deleted_table_map> ("deleted-map")[name] = &c;
          return;
        }

        string id (class_fq_name (c), 2); // Remove leading '::'.

        sema_rel::table& t (model_.new_node<sema_rel::table> (id));
        t.set ("cxx-location", c.location ());
        t.set ("class", &c);
        t.set ("deleted-map", deleted_column_map ());
        model_.new_edge<sema_rel::qnames> (model_, t, name);

        t.options (table_options (c));

        t.extra ()["kind"] =(poly == 0
                             ? "object"
                             : (poly == &c
                                ? "polymorphic root object"
                                : "polymorphic derived object"));

        // Add columns.
        //
        {
          bool tr (true); //@@ (im)persfect forwarding.
          instance<object_columns> oc (model_, t, tr);
          oc->traverse (c);
        }

        // Add indexes.
        //
        {
          instance<object_indexes> oi (model_, t);
          oi->traverse (c);
        }

        tables_.insert (name);

        // Create tables for members.
        //
        {
          instance<member_create> mc (model_);
          mc->traverse (c);
        }
      }

    protected:
      sema_rel::model& model_;
      tables tables_;
    };
  }
}

#endif // ODB_RELATIONAL_MODEL_HXX
