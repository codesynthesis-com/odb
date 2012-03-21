// file      : odb/relational/inline.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_INLINE_HXX
#define ODB_RELATIONAL_INLINE_HXX

#include <odb/relational/context.hxx>
#include <odb/relational/common.hxx>

namespace relational
{
  namespace inline_
  {
    //
    //
    struct callback_calls: traversal::class_, virtual context
    {
      typedef callback_calls base;

      callback_calls ()
      {
        *this >> inherits_ >> *this;
      }

      callback_calls (callback_calls const&)
          : root_context (), //@@ -Wextra
            context ()
      {
        *this >> inherits_ >> *this;
      }

      virtual void
      traverse (type& c, bool constant)
      {
        const_ = constant;
        traverse (c);
      }

      virtual void
      traverse (type& c)
      {
        bool obj (object (c));

        // Ignore transient bases.
        //
        if (!(obj || view (c)))
          return;

        if (c.count ("callback"))
        {
          string name (c.get<string> ("callback"));

          // In case of the const instance, we only generate the call if
          // there is a const callback.
          //
          string const& type (class_fq_name (c));

          if (const_)
          {
            if (c.count ("callback-const"))
              os << "static_cast< const " << type << "& > (x)." <<
                name << " (e, db);";
          }
          else
            os << "static_cast< " << type << "& > (x)." << name << " (e, db);";
        }
        else if (obj)
          inherits (c);
      }

    protected:
      bool const_;
      traversal::inherits inherits_;
    };

    //
    //
    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      virtual void
      traverse (type& c)
      {
        if (class_file (c) != unit.file ())
          return;

        if (object (c))
          traverse_object (c);
        if (view (c))
          traverse_view (c);
        else if (composite (c))
          traverse_composite (c);
      }

      virtual void
      object_extra (type&)
      {
      }

      virtual void
      traverse_object (type& c)
      {
        semantics::data_member* id (id_member (c));
        bool base_id (id ? &id->scope () != &c : false); // Comes from base.

        semantics::data_member* optimistic (context::optimistic (c));

        // Base class the contains the object id and version for optimistic
        // concurrency.
        //
        type* base (
          id != 0 && base_id ? dynamic_cast<type*> (&id->scope ()) : 0);

        type* poly_root (context::polymorphic (c));
        bool poly (poly_root != 0);
        bool poly_derived (poly && poly_root != &c);

        bool abst (abstract (c));
        bool reuse_abst (abst && !poly);

        string const& type (class_fq_name (c));
        string traits ("access::object_traits< " + type + " >");

        os << "// " << class_name (c) << endl
           << "//" << endl
           << endl;

        object_extra (c);

        // id (object_type)
        //
        if (id != 0 || !reuse_abst)
        {
          os << "inline" << endl
             << traits << "::id_type" << endl
             << traits << "::" << endl
             << "id (const object_type&" << (id != 0 ? " obj" : "") << ")"
             << "{";

          if (id != 0)
          {
            if (base_id)
              os << "return object_traits< " << class_fq_name (*base) <<
                " >::id (obj);";
            else
              os << "return obj." << id->name () << ";";
          }

          os << "}";
        }

        if (id != 0 && base_id)
        {
          if (!poly_derived)
          {
            // id (image_type)
            //
            if (options.generate_query ())
            {
              os << "inline" << endl
                 << traits << "::id_type" << endl
                 << traits << "::" << endl
                 << "id (const image_type& i)"
                 << "{"
                 << "return object_traits< " << class_fq_name (*base) <<
                " >::id (i);"
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
                 << "return object_traits< " << class_fq_name (*base) <<
                " >::version (i);"
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
             << "object_traits< " << class_fq_name (*base) <<
            " >::bind (b, i" << (optimistic != 0 ? ", bv" : "") << ");"
             << "}";

          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "init (id_image_type& i, const id_type& id" <<
            (optimistic != 0 ? ", const version_type* v" : "") << ")"
             << "{"
             << "object_traits< " << class_fq_name (*base) <<
            " >::init (i, id" << (optimistic != 0 ? ", v" : "") << ");"
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

        //
        // The rest does not apply to reuse-abstract objects.
        //
        if (reuse_abst)
          return;

        // erase (object_type)
        //
        if (id != 0 && !poly && optimistic == 0)
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "erase (database& db, const object_type& obj)"
             << "{"
             << "callback (db, obj, callback_event::pre_erase);"
             << "erase (db, id (obj));"
             << "callback (db, obj, callback_event::post_erase);"
             << "}";
        }

        // callback ()
        //
        os << "inline" << endl
           << "void " << traits << "::" << endl
           << "callback (database& db, object_type& x, callback_event e)"
           <<  endl
           << "{"
           << "ODB_POTENTIALLY_UNUSED (db);"
           << "ODB_POTENTIALLY_UNUSED (x);"
           << "ODB_POTENTIALLY_UNUSED (e);"
           << endl;
        callback_calls_->traverse (c, false);
        os << "}";

        os << "inline" << endl
           << "void " << traits << "::" << endl
           << "callback (database& db, const object_type& x, callback_event e)"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (db);"
           << "ODB_POTENTIALLY_UNUSED (x);"
           << "ODB_POTENTIALLY_UNUSED (e);"
           << endl;
        callback_calls_->traverse (c, true);
        os << "}";

        // query_type
        //
        if (options.generate_query ())
        {
          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type ()"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (bool v)" << endl
             << "  : query_base_type (v)"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (const char* q)" << endl
             << "  : query_base_type (q)"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (const std::string& q)" << endl
             << "  : query_base_type (q)"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (const query_base_type& q)" << endl
             << "  : query_base_type (q)"
             << "{"
             << "}";
        }

        // load_()
        //
        if (id != 0 && !(poly_derived || has_a (c, test_container)))
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "load_ (";

          if (poly && !poly_derived)
            os << "base_statements_type&, ";
          else
            os << "statements_type&, ";

          os << "object_type&)"
             << "{"
             << "}";
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
            os << "return object_traits<base_type>::root_image (*i.base);";
          else
            os << "return i;";

          os << "}";

          // clone_image ()
          //
          os << "inline" << endl
             << traits << "::image_type*" << endl
             << traits << "::" << endl
             << "clone_image (const image_type& i)"
             << "{";

          if (poly_derived)
            os << "typedef object_traits<base_type> base_traits;"
               << endl
               << "details::unique_ptr<base_traits::image_type> p (" << endl
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
             << "copy_image (image_type& d, const image_type& s)"
             << "{";

          if (poly_derived)
            os << "typedef object_traits<base_type> base_traits;"
               << endl
               << "base_traits::image_type* b (d.base);"
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
            os << "object_traits<base_type>::free_image (i->base);";

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
        string traits ("access::view_traits< " + type + " >");

        os << "// " << class_name (c) << endl
           << "//" << endl
           << endl;

        view_extra (c);

        // query_type
        //
        size_t obj_count (c.get<size_t> ("object-count"));

        if (obj_count != 0)
        {
          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type ()"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (bool v)" << endl
             << "  : query_base_type (v)"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (const char* q)" << endl
             << "  : query_base_type (q)"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (const std::string& q)" << endl
             << "  : query_base_type (q)"
             << "{"
             << "}";

          os << "inline" << endl
             << traits << "::query_type::" << endl
             << "query_type (const query_base_type& q)" << endl
             << "  : query_base_type (q)"
             << "{"
             << "}";
        }

        // callback ()
        //
        os << "inline" << endl
           << "void " << traits << "::" << endl
           << "callback (database& db, view_type& x, callback_event e)"
           <<  endl
           << "{"
           << "ODB_POTENTIALLY_UNUSED (db);"
           << "ODB_POTENTIALLY_UNUSED (x);"
           << "ODB_POTENTIALLY_UNUSED (e);"
           << endl;
        callback_calls_->traverse (c, false);
        os << "}";
      }

      virtual void
      traverse_composite (type&)
      {
      }

    private:
      instance<callback_calls> callback_calls_;
    };

    struct include: virtual context
    {
      typedef include base;

      virtual void
      generate ()
      {
        if (features.polymorphic_object && options.generate_query ())
          os << "#include <odb/details/unique-ptr.hxx>" << endl
             << endl;
      }
    };
  }
}

#endif // ODB_RELATIONAL_INLINE_HXX
