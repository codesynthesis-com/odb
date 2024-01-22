// file      : odb/relational/context.ixx
// license   : GNU GPL v3; see accompanying LICENSE file

namespace relational
{
  inline bool context::
  grow (semantics::class_& c, user_section* s)
  {
    return current ().grow_impl (c, s);
  }

  inline bool context::
  grow (semantics::data_member& m)
  {
    return current ().grow_impl (m);
  }

  inline bool context::
  grow (semantics::data_member& m,
        semantics::type& t,
        const custom_cxx_type* ct,
        string const& kp)
  {
    return current ().grow_impl (m, t, ct, kp);
  }

  inline context::string context::
  quote_string (string const& str) const
  {
    return current ().quote_string_impl (str);
  }

  inline context::string context::
  quote_id (string const& id) const
  {
    return current ().quote_id_impl (qname (id));
  }

  inline context::string context::
  quote_id (qname const& id) const
  {
    return current ().quote_id_impl (id);
  }
}
