// file      : odb/semantics/relational/changeset.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/changeset.hxx>

namespace semantics
{
  namespace relational
  {
    changeset::
    changeset (changeset const& c, qscope& b, graph& g)
        : qscope (c, &b, g),
          version_ (c.version_),
          alters_model_ (0)
    {
    }

    changeset::
    changeset (xml::parser& p, qscope& b, graph& g)
        : qscope (p, &b, g),
          version_ (p.attribute<version_type> ("version")),
          alters_model_ (0)
    {
    }

    void changeset::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "changeset");
      s.attribute ("version", version_);
      qscope::serialize_content (s);
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

          {
            type_info ti (typeid (changeset));
            ti.add_base (typeid (qscope));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
