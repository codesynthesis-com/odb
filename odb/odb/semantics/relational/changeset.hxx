// file      : odb/semantics/relational/changeset.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_CHANGESET_HXX
#define ODB_SEMANTICS_RELATIONAL_CHANGESET_HXX

#include <odb/semantics/relational/elements.hxx>

namespace semantics
{
  namespace relational
  {
    class model;
    class changeset;

    class alters_model: public edge
    {
    public:
      typedef relational::model model_type;
      typedef relational::changeset changeset_type;

      model_type&
      model () const {return *model_;}

      changeset_type&
      changeset () const {return *changeset_;}

    public:
      alters_model () : model_ (0), changeset_ (0) {}

      void
      set_left_node (changeset_type& c)
      {
        assert (changeset_ == 0);
        changeset_ = &c;
      }

      void
      set_right_node (model_type& m)
      {
        assert (model_ == 0);
        model_ = &m;
      }

    protected:
      model_type* model_;
      changeset_type* changeset_;
    };

    class changeset: public qscope
    {
    public:
      typedef relational::version version_type;

      version_type
      version () const {return version_;}

      // Returns model to which this changeset applies (as opposed to the
      // ultimate base model). Note that it may not be set.
      //
      model&
      base_model () const {return alters_model_->model ();}

    public:
      changeset (version_type v): version_ (v), alters_model_ (0) {}
      changeset (changeset const&, qscope& base, graph&);
      changeset (xml::parser&, qscope& base, graph&);

      virtual string
      kind () const {return "changeset";}

      virtual void
      serialize (xml::serializer&) const;

    public:
      virtual void
      add_edge_left (alters_model& am)
      {
        assert (alters_model_ == 0);
        alters_model_ = &am;
      }

      using qscope::add_edge_left;
      using qscope::add_edge_right;

    private:
      version_type version_;
      alters_model* alters_model_;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_CHANGESET_HXX
