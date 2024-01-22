// file      : odb/semantics/relational/foreign-key.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <ostream>
#include <istream>

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/foreign-key.hxx>

using namespace std;

namespace semantics
{
  namespace relational
  {
    static const char* action_str[] = {"NO ACTION", "CASCADE", "SET NULL"};

    ostream&
    operator<< (ostream& os, foreign_key::action_type v)
    {
      return os << action_str[v];
    }

    istream&
    operator>> (istream& is, foreign_key::action_type& v)
    {
      string s;
      getline (is, s);

      if (!is.eof ())
        is.setstate (istream::failbit);

      if (!is.fail ())
      {
        if (s == "NO ACTION")
          v = foreign_key::no_action;
        else if (s == "CASCADE")
          v = foreign_key::cascade;
        else if (s == "SET NULL")
          v = foreign_key::set_null;
        else
          is.setstate (istream::failbit);
      }

      return is;
    }

    foreign_key::
    foreign_key (foreign_key const& k, uscope& s, graph& g)
        : key (k, s, g),
          referenced_table_ (k.referenced_table_),
          referenced_columns_ (k.referenced_columns_),
          deferrable_ (k.deferrable_),
          on_delete_ (k.on_delete_)
    {
    }

    foreign_key::
    foreign_key (xml::parser& p, uscope& s, graph& g)
        : key (p, s, g),
          deferrable_ (p.attribute ("deferrable", deferrable_type ())),
          on_delete_ (p.attribute ("on-delete", no_action))
    {
      using namespace xml;

      p.next_expect (parser::start_element, xmlns, "references");
      referenced_table_ = p.attribute<qname> ("table");
      p.content (content::complex);

      for (parser::event_type e (p.peek ());
           e == parser::start_element;
           e = p.peek ())
      {
        if (p.qname () != xml::qname (xmlns, "column"))
          break; // Not our elements.

        p.next ();
        referenced_columns_.push_back (p.attribute<uname> ("name"));
        p.content (content::empty);
        p.next_expect (parser::end_element);
      }

      p.next_expect (parser::end_element);
    }

    foreign_key& foreign_key::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<foreign_key> (*this, s, g);
    }

    void foreign_key::
    serialize_attributes (xml::serializer& s) const
    {
      key::serialize_attributes (s);

      if (deferrable () != deferrable_type::not_deferrable)
        s.attribute ("deferrable", deferrable ());

      if (on_delete () != no_action)
        s.attribute ("on-delete", on_delete ());
    }

    void foreign_key::
    serialize_content (xml::serializer& s) const
    {
      key::serialize_content (s);

      // Referenced columns.
      //
      s.start_element (xmlns, "references");
      s.attribute ("table", referenced_table ());

      for (columns::const_iterator i (referenced_columns_.begin ());
           i != referenced_columns_.end (); ++i)
      {
        s.start_element (xmlns, "column");
        s.attribute ("name", *i);
        s.end_element ();
      }

      s.end_element (); // references
    }

    void foreign_key::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "foreign-key");
      serialize_attributes (s);
      serialize_content (s);
      s.end_element (); // foreign-key
    }

    // add_foreign_key
    //
    add_foreign_key& add_foreign_key::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<add_foreign_key> (*this, s, g);
    }

    void add_foreign_key::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "add-foreign-key");
      foreign_key::serialize_attributes (s);
      foreign_key::serialize_content (s);
      s.end_element ();
    }

    // drop_foreign_key
    //
    drop_foreign_key::
    drop_foreign_key (xml::parser& p, uscope&, graph& g)
        : unameable (p, g)
    {
      p.content (xml::content::empty);
    }

    drop_foreign_key& drop_foreign_key::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<drop_foreign_key> (*this, s, g);
    }

    void drop_foreign_key::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "drop-foreign-key");
      unameable::serialize_attributes (s);
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
          unameable::parser_map& m (unameable::parser_map_);

          m["foreign-key"] = &unameable::parser_impl<foreign_key>;
          m["add-foreign-key"] = &unameable::parser_impl<add_foreign_key>;
          m["drop-foreign-key"] = &unameable::parser_impl<drop_foreign_key>;

          using compiler::type_info;

          // foreign_key
          //
          {
            type_info ti (typeid (foreign_key));
            ti.add_base (typeid (key));
            insert (ti);
          }

          // add_foreign_key
          //
          {
            type_info ti (typeid (add_foreign_key));
            ti.add_base (typeid (foreign_key));
            insert (ti);
          }

          // drop_foreign_key
          //
          {
            type_info ti (typeid (drop_foreign_key));
            ti.add_base (typeid (unameable));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
