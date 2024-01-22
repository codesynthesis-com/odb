// file      : odb/semantics/relational/changelog.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <vector>
#include <sstream>

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/changelog.hxx>
#include <odb/semantics/relational/model.hxx>
#include <odb/semantics/relational/changeset.hxx>

using namespace std;

namespace semantics
{
  namespace relational
  {
    changelog::
    changelog (xml::parser& p)
        : contains_model_ (0)
    {
      using namespace xml;

      p.next_expect (parser::start_element, xmlns, "changelog");
      p.content (content::complex);

      if (p.attribute<unsigned int> ("version") != 1)
        throw parsing (p, "unsupported changelog format version");

      database_ = p.attribute ("database");
      schema_name_ = p.attribute ("schema-name", "");

      // Because things are stored in the reverse order, first save the
      // changesets as XML chunks and then re-parse them in the reverse
      // order. We have to do it this way so that we can do lookups along
      // the alters edges.
      //
      typedef vector<string> changesets;
      changesets cs;

      for (parser::event_type e (p.peek ());
           e == parser::start_element;
           e = p.peek ())
      {
        if (p.qname () != xml::qname (xmlns, "changeset"))
          break; // Not our elements.

        ostringstream os;
        os.exceptions (ios_base::badbit | ios_base::failbit);
        serializer s (os, "changeset", 0); // No pretty-printing.
        size_t depth (0);

        do
        {
          switch (p.next ())
          {
          case parser::start_element:
            {
              s.start_element (p.qname ());

              if (depth == 0)
                s.namespace_decl (xmlns, "");

              typedef parser::attribute_map_type attr_map;
              attr_map const& am (p.attribute_map ());

              for (attr_map::const_iterator i (am.begin ());
                   i != am.end (); ++i)
                s.attribute (i->first, i->second.value);

              depth++;
              break;
            }
          case parser::end_element:
            {
              depth--;
              s.end_element ();
              break;
            }
          case parser::characters:
            {
              s.characters (p.value ());
              break;
            }
          default:
            {
              depth = 0;
              break;
            }
          }
        } while (depth != 0);

        cs.push_back (os.str ());
      }

      // Get the model.
      //
      p.next_expect (parser::start_element, xmlns, "model");
      model_type& m (new_node<model_type> (p, *this));
      new_edge<contains_model_type> (*this, m);
      p.next_expect (parser::end_element);

      // Re-parse the changesets in reverse order.
      //
      qscope* base (&m);
      for (changesets::reverse_iterator i (cs.rbegin ()); i != cs.rend (); ++i)
      {
        istringstream is (*i);
        is.exceptions (ios_base::badbit | ios_base::failbit);
        parser ip (is, p.input_name ());

        ip.next_expect (parser::start_element, xmlns, "changeset");

        changeset& c (new_node<changeset> (ip, *base, *this));
        new_edge<contains_changeset> (*this, c);
        base = &c;

        ip.next_expect (parser::end_element);
      }

      p.next_expect (parser::end_element);
    }

    void changelog::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "changelog");
      s.namespace_decl (xmlns, "");
      s.attribute ("database", database_);
      if (!schema_name_.empty ())
        s.attribute ("schema-name", schema_name_);
      s.attribute ("version", 1); // Format version.

      // For better readability serialize things in reverse order so that
      // the most recent changeset appears first.
      //
      for (contains_changeset_list::const_reverse_iterator i (
             contains_changeset_.rbegin ());
           i != contains_changeset_.rend (); ++i)
      {
        (*i)->changeset ().serialize (s);
        s.characters ("\n");
      }

      model ().serialize (s);
      s.end_element ();
    }

    // type info
    //
    namespace
    {
      struct init
      {
        init ()
        {
          using compiler::type_info;

          // contains_model
          //
          {
            type_info ti (typeid (contains_model));
            ti.add_base (typeid (edge));
            insert (ti);
          }

          // contains_changeset
          //
          {
            type_info ti (typeid (contains_changeset));
            ti.add_base (typeid (edge));
            insert (ti);
          }

          // changelog
          //
          {
            type_info ti (typeid (changelog));
            ti.add_base (typeid (node));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
