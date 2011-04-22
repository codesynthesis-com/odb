// file      : odb/relational/inline.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_INLINE_HXX
#define ODB_RELATIONAL_INLINE_HXX

#include <odb/relational/context.hxx>
#include <odb/relational/common.hxx>

namespace relational
{
  namespace inline_
  {
    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      virtual void
      traverse (type& c)
      {
        if (c.file () != unit.file ())
          return;

        if (c.count ("object"))
          traverse_object (c);
        else if (comp_value (c))
          traverse_value (c);
      }

      virtual void
      traverse_object (type& c)
      {
        string const& type (c.fq_name ());
        string traits ("access::object_traits< " + type + " >");

        semantics::data_member& id (id_member (c));
        bool base_id (&id.scope () != &c); // Id comes from a base class.

        os << "// " << c.name () << endl
           << "//" << endl
           << endl;

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

        // id (object_type)
        //
        os << "inline" << endl
           << traits << "::id_type" << endl
           << traits << "::" << endl
           << "id (const object_type& obj)"
           << "{";

        if (base_id)
          os << "return object_traits< " << id.scope ().fq_name () <<
            " >::id (obj);";
        else
          os << "return obj." << id.name () << ";";

        os << "}";

        // id (image_type)
        //
        if (options.generate_query () && base_id)
        {
          os << "inline" << endl
             << traits << "::id_type" << endl
             << traits << "::" << endl
             << "id (const image_type& i)"
             << "{"
             << "return object_traits< " << id.scope ().fq_name () <<
            " >::id (i);"
             << "}";
        }

        // bind (id_image_type)
        //
        if (base_id)
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "bind (" << bind_vector << " b, id_image_type& i)"
             << "{"
             << "object_traits< " << id.scope ().fq_name () <<
            " >::bind (b, i);"
             << "}";
        }

        if (base_id)
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "init (id_image_type& i, const id_type& id)"
             << "{"
             << "object_traits< " << id.scope ().fq_name () <<
            " >::init (i, id);"
             << "}";
        }

        // load_()
        //
        if (!has_a (c, test_container))
        {
          os << "inline" << endl
             << "void " << traits << "::" << endl
             << "load_ (" << db << "::object_statements< object_type >&, " <<
            "object_type&)"
             << "{"
             << "}";
        }
      }

      virtual void
      traverse_value (type&)
      {
        /*
          string const& type (c.fq_name ());
          string traits ("access::composite_value_traits< " + type + " >");

          os << "// " << c.name () << endl
          << "//" << endl
          << endl;

        */
      }
    };
  }
}

#endif // ODB_RELATIONAL_INLINE_HXX
