// file      : odb/semantics/relational/key.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/key.hxx>
#include <odb/semantics/relational/column.hxx>

namespace semantics
{
  namespace relational
  {
    key::
    key (key const& k, uscope& s, graph& g)
        : unameable (k, g)
    {
      for (contains_iterator i (k.contains_begin ());
           i != k.contains_end (); ++i)
      {
        column* c (s.lookup<column, drop_column> (i->column ().name ()));
        assert (c != 0);
        g.new_edge<contains> (*this, *c, i->options ());
      }
    }

    key::
    key (xml::parser& p, uscope& s, graph& g)
        : unameable (p, g)
    {
      using namespace xml;
      p.content (content::complex);

      for (parser::event_type e (p.peek ());
           e == parser::start_element;
           e = p.peek ())
      {
        if (p.qname () != xml::qname (xmlns, "column"))
          break; // Not our elements.

        p.next ();
        p.content (content::empty);

        uname n (p.attribute<uname> ("name"));
        column* c (s.lookup<column, drop_column> (n));
        if (c == 0)
          throw parsing (p, "invalid column name in the 'name' attribute");

        string o (p.attribute ("options", string ()));
        g.new_edge<contains> (*this, *c, o);

        p.next_expect (parser::end_element);
      }
    }

    void key::
    serialize_content (xml::serializer& s) const
    {
      for (contains_iterator i (contains_begin ()); i != contains_end (); ++i)
      {
        s.start_element (xmlns, "column");
        s.attribute ("name", i->column ().name ());
        if (!i->options ().empty ())
          s.attribute ("options", i->options ());
        s.end_element ();
      }
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

          // contains
          //
          {
            type_info ti (typeid (contains));
            ti.add_base (typeid (edge));
            insert (ti);
          }

          // key
          //
          {
            type_info ti (typeid (key));
            ti.add_base (typeid (node));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
