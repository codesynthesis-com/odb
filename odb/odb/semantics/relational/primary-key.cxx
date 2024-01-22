// file      : odb/semantics/relational/primary-key.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/primary-key.hxx>

namespace semantics
{
  namespace relational
  {
    primary_key::
    primary_key (primary_key const& k, uscope& s, graph& g)
        : key (k, s, g), auto__ (k.auto__), extra_map_ (k.extra_map_)
    {
    }

    primary_key::
    primary_key (xml::parser& p, uscope& s, graph& g)
        : key (p, s, g),
          auto__ (p.attribute ("auto", false))
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

    primary_key& primary_key::
    clone (uscope& s, graph& g) const
    {
      return g.new_node<primary_key> (*this, s, g);
    }

    void primary_key::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "primary-key");
      key::serialize_attributes (s);

      if (auto_ ())
        s.attribute ("auto", true);

      for (extra_map::const_iterator i (extra_map_.begin ());
           i != extra_map_.end (); ++i)
        s.attribute (i->first, i->second);

      key::serialize_content (s);
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
          unameable::parser_map_["primary-key"] =
            &unameable::parser_impl<primary_key>;

          using compiler::type_info;

          {
            type_info ti (typeid (primary_key));
            ti.add_base (typeid (key));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
