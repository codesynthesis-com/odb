// file      : odb/relational/inline.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_INLINE_HXX
#define ODB_RELATIONAL_INLINE_HXX

#include <odb/diagnostics.hxx>
#include <odb/relational/context.hxx>
#include <odb/relational/common.hxx>

namespace relational
{
  namespace inline_
  {
    //
    // get/set null (composite value only)
    //

    struct null_member: virtual member_base
    {
      typedef null_member base;

      null_member (bool get)
          : member_base (0, 0, string (), string ()), get_ (get) {}

    protected:
      bool get_;
    };

    template <typename T>
    struct null_member_impl: null_member, virtual member_base_impl<T>
    {
      typedef null_member_impl base_impl;

      null_member_impl (base const& x): base (x) {}

      typedef typename member_base_impl<T>::member_info member_info;

      virtual bool
      pre (member_info& mi)
      {
        // If the member is soft- added or deleted, check the version.
        //
        unsigned long long av (added (mi.m));
        unsigned long long dv (deleted (mi.m));
        if (av != 0 || dv != 0)
        {
          os << "if (";

          if (av != 0)
            os << "svm >= schema_version_migration (" << av << "ULL, true)";

          if (av != 0 && dv != 0)
            os << " &&" << endl;

          if (dv != 0)
            os << "svm <= schema_version_migration (" << dv << "ULL, true)";

          os << ")"
             << "{";
        }

        // If the whole value type is readonly, then set will never be
        // called with sk == statement_update.
        //
        if (!get_ && !readonly (*context::top_object))
        {
          semantics::class_* c;

          if (readonly (mi.m) || ((c = composite (mi.t)) && readonly (*c)))
            os << "if (sk == statement_insert)" << endl;
        }

        return true;
      }

      virtual void
      post (member_info& mi)
      {
        if (added (mi.m) || deleted (mi.m))
          os << "}";
      }

      virtual void
      traverse_composite (member_info& mi)
      {
        string traits ("composite_value_traits< " + mi.fq_type () + ", id_" +
                       db.string () + " >");

        if (get_)
          os << "r = r && " << traits << "::get_null (" <<
            "i." << mi.var << "value";
        else
          os << traits << "::set_null (i." << mi.var << "value, sk";

        if (versioned (*composite (mi.t)))
          os << ", svm";

        os << ");";
      }
    };

    struct null_base: traversal::class_, virtual context
    {
      typedef null_base base;

      null_base (bool get): get_ (get) {}

      virtual void
      traverse (type& c)
      {
        // Ignore transient bases.
        //
        if (!composite (c))
          return;

        string traits ("composite_value_traits< " + class_fq_name (c) +
                       ", id_" + db.string () + " >");

        // If the derived value type is readonly, then set will never be
        // called with sk == statement_update.
        //
        if (!get_ && readonly (c) && !readonly (*context::top_object))
          os << "if (sk == statement_insert)" << endl;

        if (get_)
          os << "r = r && " << traits << "::get_null (i";
        else
          os << traits << "::set_null (i, sk";

        if (versioned (c))
          os << ", svm";

        os << ");";
      }

    protected:
      bool get_;
    };

    //
    //
    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      class_ ()
          : typedefs_ (false),
            get_null_base_ (true),
            get_null_member_ (true),
            set_null_base_ (false),
            set_null_member_ (false)
      {
        init ();
      }

      class_ (class_ const&)
          : root_context (), //@@ -Wextra
            context (),
            typedefs_ (false),
            get_null_base_ (true),
            get_null_member_ (true),
            set_null_base_ (false),
            set_null_member_ (false)
      {
        init ();
      }

      void
      init ()
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;

        get_null_base_inherits_ >> get_null_base_;
        get_null_member_names_ >> get_null_member_;

        set_null_base_inherits_ >> set_null_base_;
        set_null_member_names_ >> set_null_member_;
      }

      virtual void
      traverse (type& c)
      {
        class_kind_type ck (class_kind (c));

        if (ck == class_other ||
            (!options.at_once () && class_file (c) != unit.file ()))
          return;

        names (c);

        context::top_object = context::cur_object = &c;

        switch (ck)
        {
        case class_object: traverse_object (c); break;
        case class_view: traverse_view (c); break;
        case class_composite: traverse_composite (c); break;
        default: break;
        }

        context::top_object = context::cur_object = 0;
      }

      virtual void
      object_extra (type&)
      {
      }

      virtual void
      traverse_object (type& c)
      {
        using semantics::data_member;

        data_member_path* id (id_member (c));
        data_member* idf (id ? id->front () : 0);
        bool auto_id (id && auto_ (*id));
        bool base_id (id && &idf->scope () != &c); // Comes from base.
        data_member* optimistic (context::optimistic (c));

        // Base class that contains the object id and version for optimistic
        // concurrency.
        //
        type* base (base_id ? dynamic_cast<type*> (&idf->scope ()) : 0);

        type* poly_root (context::polymorphic (c));
        bool poly (poly_root != 0);
        bool poly_derived (poly && poly_root != &c);

        bool abst (abstract (c));
        bool reuse_abst (abst && !poly);

        bool versioned (context::versioned (c));

        string const& type (class_fq_name (c));
        string traits ("access::object_traits_impl< " + type + ", id_" +
                       db.string () + " >");

        user_sections& uss (c.get<user_sections> ("user-sections"));

        os << "// " << class_name (c) << endl
           << "//" << endl
           << endl;

        object_extra (c);

        if (id != 0 && base_id)
        {
          if (!poly_derived)
          {
            // id (id_image_type)
            //
            if (auto_id)
            {
              os << "inline" << endl
                 << traits << "::id_type" << endl
                 << traits << "::" << endl
                 << "id (const id_image_type& i)"
                 << "{"
                 << "return object_traits_impl< " << class_fq_name (*base) <<
                ", id_" << db << " >::id (i);"
                 << "}";
            }

            // id (image_type)
            //
            if (options.generate_query ())
            {
              os << "inline" << endl
                 << traits << "::id_type" << endl
                 << traits << "::" << endl
                 << "id (const image_type& i)"
                 << "{"
                 << "return object_traits_impl< " << class_fq_name (*base) <<
                ", id_" << db << " >::id (i);"
                 << "}";
            }

            // version (image_type)
            //
            if (optimistic != 0)
            {
              os << "inline" << endl
                 << traits << "::version_type" << endl
                 << traits << "::" << endl
                 << "version (const image_type& i)"
                 << "{"
                 << "return object_traits_impl< " << class_fq_name (*base) <<
                ", id_" << db << " >::version (i);"
                 << "}";
            }
          }

          // bind (id_image_type)
          //
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "bind (" << bind_vector << " b, id_image_type& i" <<
            (optimistic != 0 ? ", bool bv" : "") << ")"
             << "{"
             << "object_traits_impl< " << class_fq_name (*base) << ", id_" <<
            db << " >::bind (b, i" << (optimistic != 0 ? ", bv" : "") << ");"
             << "}";

          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "init (id_image_type& i, const id_type& id" <<
            (optimistic != 0 ? ", const version_type* v" : "") << ")"
             << "{"
             << "object_traits_impl< " << class_fq_name (*base) << ", id_" <<
            db << " >::init (i, id" << (optimistic != 0 ? ", v" : "") << ");"
             << "}";
        }

        if (poly_derived)
        {
          size_t depth (polymorphic_depth (c));

          // check_version
          //
          os << "inline" << endl
             << "bool " << traits << "::" << endl
             << "check_version (const std::size_t* v, const image_type& i)"
             << "{"
             << "return ";

          string image ("i.");
          for (size_t i (0); i < depth; ++i)
          {
            os << (i == 0 ? "" : " ||") << endl
               << "  v[" << i << "UL] != " << image << "version";

            image += "base->";
          }

          os << ";"
             << "}";

          // update_version
          //
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "update_version (std::size_t* v, const image_type& i, " <<
            db << "::binding* b)"
             << "{";

          image = "i.";
          for (size_t i (0); i < depth; ++i)
          {
            os << "v[" << i << "UL] = " << image << "version;";
            image += "base->";
          }

          // A poly-abstract class always has only one entry in the
          // bindings array.
          //
          if (abst)
            os << "b[0].version++;";
          else
            for (size_t i (0); i < depth; ++i)
              os << "b[" << i << "UL].version++;";

          os << "}";
        }

        // The rest does not apply to reuse-abstract objects.
        //
        if (reuse_abst)
          return;

        // erase (object_type)
        //
        if (id != 0 && !poly && optimistic == 0 &&
            !has_a (c, test_smart_container))
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "erase (database& db, const object_type& obj)"
             << "{"
             << "callback (db, obj, callback_event::pre_erase);"
             << "erase (db, id (obj));";

          // Note that we don't reset sections since the object is now
          // transient and the state of a section in a transient object
          // is undefined.

          os << "callback (db, obj, callback_event::post_erase);"
             << "}";
        }

        // load (section) [thunk version; poly_derived is true]
        //
        if (uss.count (user_sections::count_total |
                       user_sections::count_load  |
                       (poly ? user_sections::count_load_empty : 0)) != 0 &&
            uss.count (user_sections::count_new   |
                       user_sections::count_load  |
                       (poly ? user_sections::count_load_empty : 0)) == 0)
        {
          os << "inline" << endl
             << "bool " << traits << "::" << endl
             << "load (connection& conn, object_type& obj, section& s, " <<
            "const info_type* pi)"
             << "{"
             << "return base_traits::load (conn, obj, s, pi);"
             << "}";
        }

        // update (section) [thunk version; poly_derived is true]
        //
        if (uss.count (user_sections::count_total  |
                       user_sections::count_update |
                       (poly ? user_sections::count_update_empty : 0)) != 0 &&
            uss.count (user_sections::count_new    |
                       user_sections::count_update |
                       (poly ? user_sections::count_update_empty : 0)) == 0)
        {
          os << "inline" << endl
             << "bool " << traits << "::" << endl
             << "update (connection& conn, const object_type& obj, " <<
            "const section& s, const info_type* pi)"
             << "{"
             << "return base_traits::update (conn, obj, s, pi);"
             << "}";
        }

        // load_()
        //
        if (id != 0 &&
            !(poly_derived ||
              has_a (c, test_container | include_eager_load, &main_section) ||
              uss.count (user_sections::count_new  |
                         user_sections::count_load |
                         (poly ? user_sections::count_load_empty : 0)) != 0))
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "load_ (statements_type& sts," << endl
             << "object_type& obj," << endl
             << "bool";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (sts);"
             << "ODB_POTENTIALLY_UNUSED (obj);";

          if (versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);";

          os << endl;

          // Mark eager sections as loaded.
          //
          for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
          {
            // Skip special sections.
            //
            if (i->special == user_section::special_version)
              continue;

            data_member& m (*i->member);

            // If the section is soft- added or deleted, check the version.
            // We can only end up here if the object itself is versioned
            // (simple value section).
            //
            unsigned long long av (added (m));
            unsigned long long dv (deleted (m));
            if (av != 0 || dv != 0)
            {
              os << "if (";

              if (av != 0)
                os << "svm >= schema_version_migration (" << av << "ULL, true)";

              if (av != 0 && dv != 0)
                os << " &&" << endl;

              if (dv != 0)
                os << "svm <= schema_version_migration (" << dv << "ULL, true)";

              os << ")" << endl;
            }

            // Section access is always by reference.
            //
            member_access& ma (m.get<member_access> ("get"));
            if (!ma.synthesized)
              os << "// From " << location_string (ma.loc, true) << endl;

            os << ma.translate ("obj") << ".reset (true, false);"
               << endl;
          }

          os << "}";
        }

        if (poly && need_image_clone && options.generate_query ())
        {
          // root_image ()
          //
          os << "inline" << endl
             << traits << "::root_traits::image_type&" << endl
             << traits << "::" << endl
             << "root_image (image_type& i)"
             << "{";

          if (poly_derived)
            os << "return base_traits::root_image (*i.base);";
          else
            os << "return i;";

          os << "}";

          // clone_image ()
          //
          os << "inline" << endl
             << traits << "::image_type*" << endl
             << traits << "::" << endl
             << "clone_image (image_type& i)"
             << "{";

          if (poly_derived)
            os << "details::unique_ptr<base_traits::image_type> p (" << endl
               << "base_traits::clone_image (*i.base));"
               << "image_type* c (new image_type (i));"
               << "c->base = p.release ();"
               << "return c;";
          else
            os << "return new image_type (i);";

          os << "}";

          // copy_image ()
          //
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "copy_image (image_type& d, image_type& s)"
             << "{";

          if (poly_derived)
            os << "base_traits::image_type* b (d.base);"
               << "base_traits::copy_image (*b, *s.base);"
               << "d = s;" // Overwrites the base pointer.
               << "d.base = b;";
          else
            os << "d = s;";

          os << "}";

          // free_image ()
          //
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "free_image (image_type* i)"
             << "{";

          if (poly_derived)
            os << "base_traits::free_image (i->base);";

          os << "delete i;"
             << "}";
        }
      }

      virtual void
      view_extra (type&)
      {
      }

      virtual void
      traverse_view (type& c)
      {
        string const& type (class_fq_name (c));
        string traits ("access::view_traits_impl< " + type + ", id_" +
                       db.string () + " >");

        os << "// " << class_name (c) << endl
           << "//" << endl
           << endl;

        view_extra (c);
      }

      virtual void
      traverse_composite (type& c)
      {
        bool versioned (context::versioned (c));

        string const& type (class_fq_name (c));
        string traits ("access::composite_value_traits< " + type + ", id_" +
                       db.string () + " >");

        os << "// " << class_name (c) << endl
           << "//" << endl
           << endl;

        if (!has_a (c, test_container))
        {
          // get_null (image)
          //
          os << "inline" << endl
             << "bool " << traits << "::" << endl
             << "get_null (const image_type& i";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{";

          if (versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);"
               << endl;

          os << "bool r (true);";

          inherits (c, get_null_base_inherits_);
          names (c, get_null_member_names_);

          os << "return r;"
             << "}";

          // set_null (image)
          //
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "set_null (image_type& i," << endl
             << db << "::statement_kind sk";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (sk);";

          if (versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);";

          os << endl
             << "using namespace " << db << ";"
             << endl;

          inherits (c, set_null_base_inherits_);
          names (c, set_null_member_names_);

          os << "}";
        }
      }

    private:
      traversal::defines defines_;
      typedefs typedefs_;

      instance<null_base> get_null_base_;
      traversal::inherits get_null_base_inherits_;
      instance<null_member> get_null_member_;
      traversal::names get_null_member_names_;

      instance<null_base> set_null_base_;
      traversal::inherits set_null_base_inherits_;
      instance<null_member> set_null_member_;
      traversal::names set_null_member_names_;
    };

    struct include: virtual context
    {
      typedef include base;

      virtual void
      generate ()
      {
        if (versioned ())
          os << "#include <odb/schema-version.hxx>" << endl
             << endl;

        if (features.polymorphic_object && options.generate_query ())
          os << "#include <odb/details/unique-ptr.hxx>" << endl
             << endl;
      }
    };
  }
}

#endif // ODB_RELATIONAL_INLINE_HXX
