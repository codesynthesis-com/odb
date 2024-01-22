// file      : odb/semantics/relational/model.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_MODEL_HXX
#define ODB_SEMANTICS_RELATIONAL_MODEL_HXX

#include <odb/semantics/relational/elements.hxx>

namespace semantics
{
  namespace relational
  {
    class model: public graph, public qscope
    {
    public:
      typedef relational::version version_type;

      version_type
      version () const {return version_;}

      void
      version (version_type v) {version_ = v;}

    public:
      model (version_type v): version_ (v) {}
      model (model const&, graph&);
      model (xml::parser&, graph&);

      virtual string
      kind () const {return "model";}

      virtual void
      serialize (xml::serializer&) const;

    public:
      using qscope::add_edge_left;
      using qscope::add_edge_right;

    private:
      model (model const&);
      model& operator= (model const&);

    private:
      version_type version_;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_MODEL_HXX
