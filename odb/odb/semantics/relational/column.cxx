// file      : odb/semantics/relational/column.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/column.hxx>

namespace semantics
{
  namespace relational
  {
    // column
    //
    column::
    column (column const& c, uscope&, graph& g)
        : unameable (c, g),
          type_ (c.type_),
          null_ (c.null_),
          default__ (c.default__),
          options_ (c.options_)
    {
    }

    column::
    column (xml::parser& p, uscope&, graph& g)
        : unameable (p, g),
          type_ (p.attribute ("type", string ())),
          null_ (p.attribute<bool> ("null")),
          default__ (p.attribute ("default", string ())),
          options_ (p.attribute ("options", string ()))
    {
      p.content (xml::content::empty);
    }

    column& column::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<column> (*this, s, g);
    }

    void column::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "column");
      serialize_attributes (s);
      s.end_element ();
    }

    void column::
    serialize_attributes (xml::serializer& s) const
    {
      unameable::serialize_attributes (s);

      s.attribute ("type", type ());
      s.attribute ("null", null ()); // Output even if false.

      if (!default_ ().empty ())
        s.attribute ("default", default_ ());

      if (!options ().empty ())
        s.attribute ("options", options ());
    }

    // add_column
    //
    add_column& add_column::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<add_column> (*this, s, g);
    }

    void add_column::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "add-column");
      column::serialize_attributes (s);
      s.end_element ();
    }

    // drop_column
    //
    drop_column::
    drop_column (xml::parser& p, uscope&, graph& g)
        : unameable (p, g)
    {
      p.content (xml::content::empty);
    }

    drop_column& drop_column::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<drop_column> (*this, s, g);
    }

    void drop_column::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "drop-column");
      unameable::serialize_attributes (s);
      s.end_element ();
    }

    // alter_column
    //
    alter_column::
    alter_column (alter_column const& ac, uscope& s, graph& g)
        : column (ac, s, g),
          alters_ (0),
          null_altered_ (ac.null_altered_)
    {
      column* b (s.lookup<column, drop_column> (ac.name ()));
      assert (b != 0);
      g.new_edge<alters> (*this, *b);
    }

    alter_column::
    alter_column (xml::parser& p, uscope& s, graph& g)
        : column (p, s, g),
          alters_ (0),
          null_altered_ (p.attribute_present ("null"))
    {
      name_type n (p.attribute<name_type> ("name"));
      column* b (s.lookup<column, drop_column> (n));
      assert (b != 0);
      g.new_edge<alters> (*this, *b);
    }

    alter_column& alter_column::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<alter_column> (*this, s, g);
    }

    void alter_column::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "alter-column");

      // Here we override the standard column logic.
      //
      unameable::serialize_attributes (s);

      if (null_altered_)
        s.attribute ("null", null_);

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

          m["column"] = &unameable::parser_impl<column>;
          m["add-column"] = &unameable::parser_impl<add_column>;
          m["drop-column"] = &unameable::parser_impl<drop_column>;
          m["alter-column"] = &unameable::parser_impl<alter_column>;

          using compiler::type_info;

          // column
          //
          {
            type_info ti (typeid (column));
            ti.add_base (typeid (unameable));
            insert (ti);
          }

          // add_column
          //
          {
            type_info ti (typeid (add_column));
            ti.add_base (typeid (column));
            insert (ti);
          }

          // drop_column
          //
          {
            type_info ti (typeid (drop_column));
            ti.add_base (typeid (unameable));
            insert (ti);
          }

          // alter_column
          //
          {
            type_info ti (typeid (alter_column));
            ti.add_base (typeid (column));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
