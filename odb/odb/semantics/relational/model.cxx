// file      : odb/semantics/relational/model.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/model.hxx>

namespace semantics
{
  namespace relational
  {
    model::
    model (model const& m, graph& g)
        : qscope (m, 0, g),
          version_ (m.version_)
    {
    }

    model::
    model (xml::parser& p, graph& g)
        : qscope (p, 0, g),
          version_ (p.attribute<version_type> ("version"))
    {
    }

    void model::
    serialize (xml::serializer& s) const
    {
      s.start_element (xmlns, "model");
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
            type_info ti (typeid (model));
            ti.add_base (typeid (qscope));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
