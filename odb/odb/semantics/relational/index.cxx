// file      : odb/semantics/relational/index.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/index.hxx>

namespace semantics
{
  namespace relational
  {
    // index
    //
    index::
    index (index const& i, uscope& s, graph& g)
        : key (i, s, g),
          type_ (i.type_),
          method_ (i.method_),
          options_ (i.options_)
    {
    }

    index::
    index (xml::parser& p, uscope& s, graph& g)
        : key (p, s, g),
          type_ (p.attribute ("type", string ())),
          method_ (p.attribute ("method", string ())),
          options_ (p.attribute ("options", string ()))
    {
    }

    index& index::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<index> (*this, s, g);
    }

    void index::
    serialize_attributes (xml::serializer& s) const
    {
      key::serialize_attributes (s);

      if (!type ().empty ())
        s.attribute ("type", type ());

      if (!method ().empty ())
        s.attribute ("method", method ());

      if (!options ().empty ())
        s.attribute ("options", options ());
    }

    void index::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "index");
      serialize_attributes (s);
      key::serialize_content (s);
      s.end_element ();
    }

    // add_index
    //
    add_index& add_index::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<add_index> (*this, s, g);
    }

    void add_index::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "add-index");
      index::serialize_attributes (s);
      index::serialize_content (s);
      s.end_element ();
    }

    // drop_index
    //
    drop_index::
    drop_index (xml::parser& p, uscope&, graph& g)
        : unameable (p, g)
    {
      p.content (xml::content::empty);
    }

    drop_index& drop_index::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<drop_index> (*this, s, g);
    }

    void drop_index::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "drop-index");
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

          m["index"] = &unameable::parser_impl<index>;
          m["add-index"] = &unameable::parser_impl<add_index>;
          m["drop-index"] = &unameable::parser_impl<drop_index>;

          using compiler::type_info;

          // index
          //
          {
            type_info ti (typeid (index));
            ti.add_base (typeid (key));
            insert (ti);
          }

          // add_index
          //
          {
            type_info ti (typeid (add_index));
            ti.add_base (typeid (index));
            insert (ti);
          }

          // drop_index
          //
          {
            type_info ti (typeid (drop_index));
            ti.add_base (typeid (unameable));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
