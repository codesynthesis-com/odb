// file      : odb/relational/header.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_HEADER_HXX
#define ODB_RELATIONAL_HEADER_HXX

#include <odb/relational/context.hxx>
#include <odb/relational/common.hxx>

namespace relational
{
  namespace header
  {
    //
    // image_type
    //

    struct image_member: virtual member_base
    {
      typedef image_member base;

      image_member (string const& var = string ())
          : member_base (var, 0, 0, string (), string ()) {}

      image_member (string const& var,
                    semantics::type& t,
                    const custom_cxx_type* ct,
                    string const& fq_type,
                    string const& key_prefix)
          : member_base (var, &t, ct, fq_type, key_prefix) {}
    };

    template <typename T>
    struct image_member_impl: image_member, virtual member_base_impl<T>
    {
      typedef image_member_impl base_impl;

      image_member_impl (base const& x)
          : member_base::base (x), // virtual base
            base (x),
            member_image_type_ (base::type_override_,
                                base::custom_override_,
                                base::fq_type_override_,
                                base::key_prefix_)
      {
      }

      typedef typename member_base_impl<T>::member_info member_info;

      using member_base_impl<T>::container;

      virtual bool
      pre (member_info& mi)
      {
        if (container (mi))
          return false;

        image_type = member_image_type_->image_type (mi.m);

        if (var_override_.empty ())
          os << "// " << mi.m.name () << endl
             << "//" << endl;

        return true;
      }

      virtual void
      traverse_pointer (member_info& mi)
      {
        // Object pointers in views require special treatment.
        //
        if (view_member (mi.m))
        {
          using semantics::class_;

          class_& c (*mi.ptr);
          class_* poly_root (polymorphic (c));
          bool poly_derived (poly_root != 0 && poly_root != &c);

          if (poly_derived)
            // Use a helper to create a complete chain of images all
            // the way to the root (see libodb/odb/view-image.hxx).
            //
            os << "view_object_image<" << endl
               << "  " << class_fq_name (c) << "," << endl
               << "  " << class_fq_name (*poly_root) << "," << endl
               << "  id_" << db << " >";
          else
            os << "object_traits_impl< " << class_fq_name (c) << ", " <<
              "id_" << db << " >::image_type";

          os << " " << mi.var << "value;"
             << endl;
        }
        else
          member_base_impl<T>::traverse_pointer (mi);
      }

      virtual void
      traverse_composite (member_info& mi)
      {
        os << image_type << " " << mi.var << "value;"
           << endl;
      }

    protected:
      string image_type;
      instance<member_image_type> member_image_type_;
    };

    struct image_base: traversal::class_, virtual context
    {
      typedef image_base base;

      image_base (): first_ (true) {}

      virtual void
      traverse (type& c)
      {
        bool obj (object (c));

        // Ignore transient bases. Not used for views.
        //
        if (!(obj || composite (c)))
          return;

        if (first_)
        {
          os << ": ";
          first_ = false;
        }
        else
        {
          os << "," << endl
             << "  ";
        }

        string const& type (class_fq_name (c));

        if (obj)
          os << "object_traits_impl< " << type << ", id_" << db <<
            " >::image_type";
        else
          os << "composite_value_traits< " << type << ", id_" << db <<
            " >::image_type";
      }

    private:
      bool first_;
    };

    struct image_type: traversal::class_, virtual context
    {
      typedef image_type base;

      image_type ()
      {
        *this >> names_member_ >> member_;
      }

      image_type (image_type const&)
          : root_context (), context () //@@ -Wextra
      {
        *this >> names_member_ >> member_;
      }

      virtual void
      image_extra (type&)
      {
      }

      virtual void
      traverse (type& c)
      {
        type* poly_root (polymorphic (c));
        bool poly_derived (poly_root != 0 && poly_root != &c);

        os << "struct image_type";

        if (!view (c))
        {
          // Don't go into the base if we are a derived type in a
          // polymorphic hierarchy.
          //
          if (!poly_derived)
          {
            instance<image_base> b;
            traversal::inherits i (*b);
            inherits (c, i);
          }
        }

        os << "{";

        if (poly_derived)
          os << "base_traits::image_type* base;"
             << endl;

        names (c);

        // We don't need a version if this is a composite value type
        // or reuse-abstract object.
        //
        if (!(composite (c) || (abstract (c) && !polymorphic (c))))
          os << "std::size_t version;"
             << endl;

        image_extra (c);

        os << "};";
      }

    private:
      instance<image_member> member_;
      traversal::names names_member_;
    };

    // Member-specific traits types for container members.
    //
    struct container_traits: object_members_base, virtual context
    {
      typedef container_traits base;

      container_traits (semantics::class_& c)
          : object_members_base (true, false, false), c_ (c)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member&, semantics::class_&)
      {
        // We don't want to traverse composite id.
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        if (object (c_))
          object_members_base::traverse_composite (m, c);
        else
        {
          // If we are generating traits for a composite value type, then
          // we don't want to go into its bases or it composite members.
          //
          if (m == 0 && &c == &c_)
            names (c);
        }
      }

      virtual void
      container_public_extra_pre (semantics::data_member&, semantics::type&)
      {
      }

      virtual void
      container_public_extra_post (semantics::data_member&, semantics::type&)
      {
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type& c)
      {
        using semantics::type;
        using semantics::class_;

        // Figure out if this member is from a base object or composite
        // value and if it's from an object, whether it is reuse-abstract.
        //
        bool base, reuse_abst;

        if (object (c_))
        {
          base = cur_object != &c_ ||
            !object (dynamic_cast<type&> (m.scope ()));
          reuse_abst = abstract (c_) && !polymorphic (c_);
        }
        else
        {
          base = false;      // We don't go into bases.
          reuse_abst = true; // Always abstract.
        }

        container_kind_type ck (container_kind (c));

        const custom_cxx_type* vct (0);
        const custom_cxx_type* ict (0);
        const custom_cxx_type* kct (0);

        type& vt (container_vt (m, &vct));
        type* it (0);
        type* kt (0);

        bool ordered (false);
        bool inverse (context::inverse (m, "value"));

        switch (ck)
        {
        case ck_ordered:
          {
            if (!unordered (m))
            {
              it = &container_it (m, &ict);
              ordered = true;
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            kt = &container_kt (m, &kct);
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        bool smart (!inverse &&
                    (ck != ck_ordered || ordered) &&
                    container_smart (c));

        string name (flat_prefix_ + public_name (m) + "_traits");

        // Figure out column counts.
        //
        size_t id_columns, value_columns, data_columns, cond_columns;
        bool versioned (context::versioned (m));

        if (!reuse_abst)
        {
          type& idt (container_idt (m));

          if (class_* idc = composite_wrapper (idt))
            id_columns = column_count (*idc).total;
          else
            id_columns = 1;

          data_columns = cond_columns = id_columns;

          switch (ck)
          {
          case ck_ordered:
            {
              // Add one for the index.
              //
              if (ordered)
              {
                data_columns++;

                if (smart)
                  cond_columns++;
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              // Add some for the key.
              //
              size_t n;

              class_* ptr (object_pointer (*kt));
              semantics::type& t (ptr == 0 ? *kt : utype (*id_member (*ptr)));

              if (class_* comp = composite_wrapper (t))
                n = column_count (*comp).total;
              else
                n = 1;

              data_columns += n;

              // Key is not currently used (see also bind()).
              //
              // cond_columns += n;

              break;
            }
          case ck_set:
          case ck_multiset:
            {
              // Not currently used (see also bind())
              //
              // Value is also a key.
              //
              // class_* ptr (object_pointer (vt));
              // semantics::type& t (ptr == 0 ? vt : utype (*id_member (*ptr)));
              //
              // if (class_* comp = composite_wrapper (t))
              //   cond_columns += column_count (*comp).total;
              // else
              //   cond_columns++;
              //
              break;
            }
          }

          {
            class_* ptr (object_pointer (vt));
            semantics::type& t (ptr == 0 ? vt : utype (*id_member (*ptr)));

            if (class_* comp = composite_wrapper (t))
              value_columns = column_count (*comp).total;
            else
              value_columns = 1;

            data_columns += value_columns;
          }

          // Store column counts for the source generator.
          //
          m.set ("id-column-count", id_columns);
          m.set ("value-column-count", value_columns);
          m.set ("cond-column-count", cond_columns);
          m.set ("data-column-count", data_columns);
        }

        os << "// " << m.name () << endl
           << "//" << endl
           << "struct " << exp << name;

        if (base)
        {
          semantics::class_& b (dynamic_cast<semantics::class_&> (m.scope ()));
          string const& type (class_fq_name (b));

          if (object (b))
            os << ": access::object_traits_impl< " << type << ", id_" <<
              db << " >::" << name;
          else
            os << ": access::composite_value_traits< " << type << ", id_" <<
              db << " >::" << public_name (m) << "_traits"; // No prefix_.
        }

        os << "{";

        container_public_extra_pre (m, c);

        if (!reuse_abst)
        {
          // column_count
          //
          os << "static const std::size_t id_column_count = " <<
            id_columns << "UL;";

          if (smart)
            os << "static const std::size_t value_column_count = " <<
              value_columns << "UL;"
               << "static const std::size_t cond_column_count = " <<
              cond_columns << "UL;";

          os << "static const std::size_t data_column_count = " <<
            data_columns << "UL;"
             << endl;

          os << "static const bool versioned = " << versioned << ";"
             << endl;

          // Statements.
          //
          os << "static const char insert_statement[];"
             << "static const char select_statement[];";

          if (smart)
            os << "static const char update_statement[];";

          os << "static const char delete_statement[];"
             << endl;
        }

        if (base)
        {
          container_public_extra_post (m, c);
          os << "};";

          return;
        }

        // container_type
        // container_traits
        // index_type
        // key_type
        // value_type
        //
        os << "typedef ";

        {
          semantics::names* hint;
          semantics::type& t (utype (m, hint));

          if (semantics::type* wt = wrapper (t))
          {
            // Use the hint from the wrapper unless the wrapped type is
            // qualified. In this case use the hint for the unqualified
            // type.
            //
            hint = t.get<semantics::names*> ("wrapper-hint");
            utype (*wt, hint);

            os << c.fq_name (hint);
          }
          else
            // t and c are the same.
            //
            os << t.fq_name (hint);
        }

        os << " container_type;";

        os << "typedef" << endl
           << "odb::access::container_traits<container_type>" << endl
           << "container_traits_type;";

        switch (ck)
        {
        case ck_ordered:
          {
            os << "typedef container_traits_type::index_type index_type;";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "typedef container_traits_type::key_type key_type;";
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        os << "typedef container_traits_type::value_type value_type;"
           << endl;

        // functions_type
        //
        switch (ck)
        {
        case ck_ordered:
          {
            os << "typedef " << (smart ? "smart_" : "") <<
              "ordered_functions<index_type, value_type> functions_type;";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "typedef map_functions<key_type, value_type> " <<
              "functions_type;";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "typedef set_functions<value_type> functions_type;";
            break;
          }
        }

        os << "typedef " << db << "::" << (smart ? "smart_" : "")
           << "container_statements< " << name << " > statements_type;"
           << endl;

        // cond_image_type (object id is taken from the object image).
        //
        // For dumb containers we use the id binding directly.
        //
        if (smart)
        {
          os << "struct cond_image_type"
             << "{";

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
              {
                os << "// index" << endl
                   << "//" << endl;
                instance<image_member> im (
                  "index_", *it, ict, "index_type", "index");
                im->traverse (m);
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "// key" << endl
                 << "//" << endl;
              instance<image_member> im ("key_", *kt, kct, "key_type", "key");
              im->traverse (m);
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              os << "// value" << endl
                 << "//" << endl;
              instance<image_member> im (
                "value_", vt, vct, "value_type", "value");
              im->traverse (m);
              break;
            }
          }

          os << "std::size_t version;"
             << "};";
        }

        // data_image_type (object id is taken from the object image)
        //
        os << "struct data_image_type"
           << "{";

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
            {
              os << "// index" << endl
                 << "//" << endl;
              instance<image_member> im (
                "index_", *it, ict, "index_type", "index");
              im->traverse (m);
            }
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "// key" << endl
               << "//" << endl;
            instance<image_member> im ("key_", *kt, kct, "key_type", "key");
            im->traverse (m);
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        os << "// value" << endl
           << "//" << endl;
        instance<image_member> im ("value_", vt, vct, "value_type", "value");
        im->traverse (m);

        os << "std::size_t version;"
           << "};";

        // bind (cond_image)
        //
        if (smart)
          os << "static void" << endl
             << "bind (" << bind_vector << "," << endl
             << "const " << bind_vector << " id," << endl
             << "std::size_t id_size," << endl
             << "cond_image_type&);"
             << endl;

        // bind (data_image)
        //
        os << "static void" << endl
           << "bind (" << bind_vector << "," << endl
           << "const " << bind_vector << " id," << endl
           << "std::size_t id_size," << endl
           << "data_image_type&";

        if (versioned)
          os << "," << endl
             << "const schema_version_migration&";

        os << ");"
           << endl;

        // bind (cond_image, data_image) (update)
        //
        if (smart)
        {
          os << "static void" << endl
             << "bind (" << bind_vector << "," << endl
             << "const " << bind_vector << " id," << endl
             << "std::size_t id_size," << endl
             << "cond_image_type&," << endl
             << "data_image_type&";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // grow ()
        //
        if (generate_grow)
        {
          os << "static void" << endl
             << "grow (data_image_type&," << endl
             << truncated_vector;

          if (versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // init (data_image)
        //
        if (!inverse)
        {
          os << "static void" << endl
             << "init (data_image_type&," << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
                os << "index_type*," << endl;
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "const key_type*," << endl;
              break;
            }
          case ck_set:
          case ck_multiset:
            break;
          }

          os << "const value_type&";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // init (cond_image)
        //
        if (smart)
        {
          os << "static void" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "init (cond_image_type&, index_type);";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              // os << "init (data_image_type&, const key_type&);";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              // os << "init (data_image_type&, const value_type&);";
              break;
            }
          }

          os << endl;
        }

        // init (data)
        //
        os << "static void" << endl
           << "init (";

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
              os << "index_type&," << endl;
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "key_type&," << endl;
            break;
          }
        case ck_set:
        case ck_multiset:
          break;
        }

        os << "value_type&," << endl;
        os << "const data_image_type&," << endl
           << "database*";

        if (versioned)
          os << "," << endl
             << "const schema_version_migration&";

        os << ");"
           << endl;

        // insert
        //
        os << "static void" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            os << "insert (index_type, const value_type&, void*);";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "insert (const key_type&, const value_type&, void*);";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "insert (const value_type&, void*);";
            break;
          }
        }

        os << endl;

        // select
        //
        os << "static bool" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            os << "select (index_type&, value_type&, void*);";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "select (key_type&, value_type&, void*);";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "select (value_type&, void*);";
            break;
          }
        }

        os << endl;

        // update
        //
        if (smart)
        {
          os << "static void" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "update (index_type, const value_type&, void*);";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              //os << "update (const key_type&, const value_type&, void*);";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              //os << "update (const value_type&, const value_type&, void*);";
              break;
            }
          }

          os << endl;
        }

        // delete_
        //
        os << "static void" << endl
           << "delete_ (";

        if (smart)
        {
          switch (ck)
          {
          case ck_ordered:
            {
              os << "index_type, ";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              break;
            }
          }
        }

        os << "void*);"
           << endl;

        // persist
        //
        if (!inverse)
        {
          os << "static void" << endl
             << "persist (const container_type&," << endl
             << "statements_type&";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // load
        //
        os << "static void" << endl
           << "load (container_type&," << endl
           << "statements_type&";

        if (versioned)
          os << "," << endl
             << "const schema_version_migration&";

        os << ");"
           << endl;

        // update
        //
        if (!(inverse || readonly (member_path_, member_scope_)))
        {
          os << "static void" << endl
             << "update (const container_type&," << endl
             << "statements_type&";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // erase
        //
        if (!inverse)
        {
          os << "static void" << endl
             << "erase (";

          if (smart)
            os << "const container_type*, ";

          os << "statements_type&);"
             << endl;
        }

        container_public_extra_post (m, c);

        os << "};";
      }

    protected:
      semantics::class_& c_;
    };

    //
    //
    struct section_traits: virtual context
    {
      typedef section_traits base;

      section_traits (semantics::class_& c): c_ (c) {}

      virtual void
      section_public_extra_pre (user_section&)
      {
      }

      virtual void
      section_public_extra_post (user_section&)
      {
      }

      virtual void
      traverse (user_section& s)
      {
        semantics::class_* poly_root (polymorphic (c_));
        bool poly (poly_root != 0);
        bool poly_derived (poly && poly_root != &c_);

        semantics::data_member* opt (optimistic (c_));

        // Treat the special version update sections as abstract in reuse
        // inheritance.
        //
        bool reuse_abst (!poly &&
                         (abstract (c_) ||
                          s.special == user_section::special_version));

        bool load (s.total != 0 && s.separate_load ());
        bool load_con (s.containers && s.separate_load ());
        bool load_opt (s.optimistic () && s.separate_load ());

        bool update (s.total != s.inverse + s.readonly); // Always separate.
        bool update_con (s.readwrite_containers);
        bool update_opt (s.optimistic () && (s.readwrite_containers || poly));

        // Don't generate anything for empty sections.
        //
        if (!(load || load_con || load_opt ||
              update || update_con || update_opt))
          return;

        // If we are adding a new section to a derived class in an optimistic
        // hierarchy, then pretend it inherits from the special version update
        // section.
        //
        user_section* rs (0);
        if (opt != 0)
        {
          // Skip overrides and get to the new section if polymorphic.
          //
          for (rs = &s; poly && rs->base != 0; rs = rs->base) ;

          if (rs != 0)
          {
            if (rs->object != &opt->scope ())
              rs->base = &(poly ? poly_root : &opt->scope ())->
                get<user_sections> ("user-sections").back ();
            else
              rs = 0;
          }
        }

        string name (public_name (*s.member) + "_traits");

        os << "// " << s.member->name () << endl
           << "//" << endl
           << "struct " << exp << name
           << "{";

        os << "typedef object_traits_impl<object_type, id_" << db <<
          ">::image_type image_type;"
           << "typedef object_traits_impl<object_type, id_" << db <<
          ">::id_image_type id_image_type;"
           << endl;

        section_public_extra_pre (s);

        // bind (id, image_type)
        //
        // If id is NULL, then id is ignored (select). Otherwise, it is
        // copied at the end (update).
        //
        if (load || load_opt || update || update_opt)
        {
          os << "static std::size_t" << endl
             << "bind (" << bind_vector << "," << endl
             << "const " << bind_vector << " id," << endl
             << "std::size_t id_size," << endl
             << "image_type&," << endl
             << db << "::statement_kind";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // grow ()
        //
        // We have to have out own version because the truncated vector
        // will have different number of elements.
        //
        if (generate_grow && (load || load_opt))
        {
          os << "static bool" << endl
             << "grow (image_type&," << endl
             << truncated_vector;

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // init (object, image)
        //
        if (load)
        {
          os << "static void" << endl
             << "init (object_type&," << endl
             << "const image_type&," << endl
             << "database*";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // init (image, object)
        //
        if (update)
        {
          os << "static " << (generate_grow ? "bool" : "void") << endl
             << "init (image_type&," << endl
             << "const object_type&";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration&";

          os << ");"
             << endl;
        }

        // The rest does not apply to reuse-abstract sections.
        //
        if (reuse_abst)
        {
          section_public_extra_post (s);
          os << "};";
          return;
        }

        // column_count
        //
        column_count_type const& cc (column_count (poly ? *poly_root : c_));

        // Generate load and update column counts even when they are zero so
        // that we can instantiate section_statements.
        //
        os << "static const std::size_t id_column_count = " << cc.id << "UL;";

        os << "static const std::size_t managed_optimistic_load_column_count" <<
          " = " << cc.optimistic_managed << "UL;"
           << "static const std::size_t load_column_count = " <<
          (load ? s.total_total () : 0) << "UL;";

        os << "static const std::size_t managed_optimistic_update_column_count" <<
          " = " << (poly_derived ? 0 : cc.optimistic_managed) << "UL;"
           << "static const std::size_t update_column_count = " <<
          (update ? s.total - s.inverse - s.readonly : 0) << "UL;"
           << endl;

        os << "static const bool versioned = " << s.versioned << ";"
           << endl;

        // Statements.
        //
        if (load || load_opt)
          os << "static const char select_statement[];"
             << endl;

        if (update || update_opt)
          os << "static const char update_statement[];"
             << endl;

        // Section statements.
        //
        if (load || load_opt || update || update_opt)
          os << "typedef " << db << "::section_statements< object_type, " <<
            name << " > statements_type;"
             << endl;

        // We pass statement cache instead of just statements because
        // we may also need statements for containers.
        //

        // load ()
        //
        if (load || load_opt || load_con)
          os << "static void" << endl
             << "load (extra_statement_cache_type&, object_type&" <<
            (poly ? ", bool top = true" : "") << ");"
             << endl;

        // update ()
        //
        if (update || update_opt || update_con)
          os << "static void" << endl
             << "update (extra_statement_cache_type&, const object_type&" <<
            (poly_derived && s.base != 0 ? ", bool base = true" : "") << ");"
             << endl;

        section_public_extra_post (s);

        os << "};";

        if (rs != 0)
          rs->base = 0;
      }

    protected:
      semantics::class_& c_;
    };

    // First pass over objects, views, and composites. Some code must be
    // split into two parts to deal with yet undefined types.
    //
    struct class1: traversal::class_, virtual context
    {
      typedef class1 base;

      class1 ()
          : typedefs_ (false),
            id_image_member_ ("id_"),
            version_image_member_ ("version_"),
            discriminator_image_member_ ("discriminator_"),
            query_columns_type_ (false, true, false),
            pointer_query_columns_type_ (true, true, false)
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;
      }

      class1 (class1 const&)
          : root_context (), //@@ -Wextra
            context (),
            typedefs_ (false),
            id_image_member_ ("id_"),
            version_image_member_ ("version_"),
            discriminator_image_member_ ("discriminator_"),
            query_columns_type_ (false, true, false),
            pointer_query_columns_type_ (true, true, false)
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;
      }

      virtual void
      traverse (type& c)
      {
        class_kind_type ck (class_kind (c));

        if (ck == class_other ||
            (!options.at_once () && class_file (c) != unit.file ()))
          return;

        names (c);

        switch (ck)
        {
        case class_object: traverse_object (c); break;
        case class_view: traverse_view (c); break;
        case class_composite: traverse_composite (c); break;
        default: break;
        }
      }

      virtual void
      object_public_extra_pre (type&)
      {
      }

      virtual void
      object_public_extra_post (type&)
      {
      }

      virtual void
      traverse_object (type&);

      virtual void
      view_public_extra_pre (type&)
      {
      }

      virtual void
      view_public_extra_post (type&)
      {
      }

      virtual void
      traverse_view (type&);

      virtual void
      traverse_composite (type&);

    private:
      traversal::defines defines_;
      typedefs typedefs_;

      instance<image_type> image_type_;
      instance<image_member> id_image_member_;
      instance<image_member> version_image_member_;
      instance<image_member> discriminator_image_member_;

      instance<query_columns_type> query_columns_type_;
      instance<query_columns_type> pointer_query_columns_type_;
    };

    // Second pass over objects, views, and composites.
    //
    struct class2: traversal::class_, virtual context
    {
      typedef class2 base;

      class2 ()
          : typedefs_ (false),
            query_columns_type_ (false, true, false),
            query_columns_type_inst_ (false, false, true),
            view_query_columns_type_ (true)
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;
      }

      class2 (class2 const&)
          : root_context (), //@@ -Wextra
            context (),
            typedefs_ (false),
            query_columns_type_ (false, true, false),
            query_columns_type_inst_ (false, false, true),
            view_query_columns_type_ (true)
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;
      }

      virtual void
      traverse (type& c)
      {
        class_kind_type ck (class_kind (c));

        if (ck == class_other ||
            (!options.at_once () && class_file (c) != unit.file ()))
          return;

        names (c);

        switch (ck)
        {
        case class_object: traverse_object (c); break;
        case class_view: traverse_view (c); break;
        case class_composite: traverse_composite (c); break;
        default: break;
        }
      }

      virtual void
      traverse_object (type& c)
      {
        if (options.generate_query ())
        {
          os << "// " << class_name (c) << endl
             << "//" << endl;

          // query_columns
          //
          // If we don't have any pointers, then query_columns is generated
          // in pass 1 (see the comment in class1 for details).
          //
          if (has_a (c, test_pointer | include_base))
            query_columns_type_->traverse (c);

          // Generate extern template declarations.
          //
          if (multi_dynamic)
            query_columns_type_inst_->traverse (c);
        }

        // Move header comment out of if-block if adding any code here.
      }

      virtual void
      traverse_view (type& c)
      {
        // query_columns
        //
        if (c.get<size_t> ("object-count") != 0)
        {
          os << "// " << class_name (c) << endl
             << "//" << endl;

          view_query_columns_type_->traverse (c);
        }

        // Move header comment out of if-block if adding any code here.
      }

      virtual void
      traverse_composite (type&)
      {
      }

    private:
      traversal::defines defines_;
      typedefs typedefs_;

      instance<query_columns_type> query_columns_type_;
      instance<query_columns_type> query_columns_type_inst_;
      instance<view_query_columns_type> view_query_columns_type_;
    };

    struct include: virtual context
    {
      typedef include base;

      virtual void
      generate ()
      {
        os << "#include <odb/details/buffer.hxx>" << endl
           << endl;

        os << "#include <odb/" << db << "/version.hxx>" << endl
           << "#include <odb/" << db << "/forward.hxx>" << endl
           << "#include <odb/" << db << "/binding.hxx>" << endl
           << "#include <odb/" << db << "/" << db << "-types.hxx>" << endl;

        if (options.generate_query ())
        {
          os << "#include <odb/" << db << "/query.hxx>" << endl;

          if (multi_dynamic)
            os << "#include <odb/" << db << "/query-dynamic.hxx>" << endl;
        }

        os << endl;
      }
    };
  }
}

#endif // ODB_RELATIONAL_HEADER_HXX
