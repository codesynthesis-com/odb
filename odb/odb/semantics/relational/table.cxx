// file      : odb/semantics/relational/table.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/table.hxx>

namespace semantics
{
  namespace relational
  {
    // table
    //
    table::
    table (table const& t, qscope& s, graph& g, bool b)
        : qnameable (t, g),
          uscope (t, (b ? s.lookup<table, drop_table> (t.name ()) : 0), g),
          options_ (t.options_),
          extra_map_ (t.extra_map_)
    {
    }

    table::
    table (xml::parser& p, qscope& s, graph& g, bool b)
        : qnameable (p, g),
          uscope (
            p,
            (b ? s.lookup<table, drop_table> (
              p.attribute<qnameable::name_type> ("name")) : 0),
            g),
          options_ (p.attribute ("options", string ()))
    {
      // All unhandled attributes go into the extra map.
      //
      typedef xml::parser::attribute_map_type attr_map;
      attr_map const& am (p.attribute_map ());

      for (attr_map::const_iterator i (am.begin ()); i != am.end (); ++i)
      {
        if (!i->second.handled)
          extra_map_[i->first.name ()] = i->second.value;
      }
    }

    table& table::
    clone (qscope& s, graph& g) const
    {
      return g.new_node<table> (*this, s, g);
    }

    void table::
    serialize_attributes (xml::serializer& s) const
    {
      qnameable::serialize_attributes (s);

      if (!options_.empty ())
        s.attribute ("options", options_);

      for (extra_map::const_iterator i (extra_map_.begin ());
           i != extra_map_.end (); ++i)
        s.attribute (i->first, i->second);
    }

    void table::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "table");
      serialize_attributes (s);
      uscope::serialize_content (s);
      s.end_element ();
    }

    // add_table
    //
    add_table& add_table::
    clone (qscope& s, graph& g) const
    {
      return g.new_node<add_table> (*this, s, g);
    }

    void add_table::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "add-table");
      table::serialize_attributes (s);
      table::serialize_content (s);
      s.end_element ();
    }

    // drop_table
    //
    drop_table::
    drop_table (xml::parser& p, qscope&, graph& g)
        : qnameable (p, g)
    {
      p.content (xml::content::empty);
    }

    drop_table& drop_table::
    clone (qscope& s, graph& g) const
    {
      return g.new_node<drop_table> (*this, s, g);
    }

    void drop_table::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "drop-table");
      qnameable::serialize_attributes (s);
      s.end_element ();
    }

    // alter_table
    //
    alter_table& alter_table::
    clone (qscope& s, graph& g) const
    {
      return g.new_node<alter_table> (*this, s, g);
    }

    void alter_table::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "alter-table");
      table::serialize_attributes (s);
      table::serialize_content (s);
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
          qnameable::parser_map& m (qnameable::parser_map_);

          m["table"] = &qnameable::parser_impl<table>;
          m["add-table"] = &qnameable::parser_impl<add_table>;
          m["drop-table"] = &qnameable::parser_impl<drop_table>;
          m["alter-table"] = &qnameable::parser_impl<alter_table>;

          using compiler::type_info;

          // table
          //
          {
            type_info ti (typeid (table));
            ti.add_base (typeid (qnameable));
            ti.add_base (typeid (uscope));
            insert (ti);
          }

          // add_table
          //
          {
            type_info ti (typeid (add_table));
            ti.add_base (typeid (table));
            insert (ti);
          }

          // drop_table
          //
          {
            type_info ti (typeid (drop_table));
            ti.add_base (typeid (qnameable));
            insert (ti);
          }

          // alter_table
          //
          {
            type_info ti (typeid (alter_table));
            ti.add_base (typeid (table));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
