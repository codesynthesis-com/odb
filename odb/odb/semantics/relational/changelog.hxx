// file      : odb/semantics/relational/changelog.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_CHANGELOG_HXX
#define ODB_SEMANTICS_RELATIONAL_CHANGELOG_HXX

#include <odb/semantics/relational/elements.hxx>

namespace semantics
{
  namespace relational
  {
    class model;
    class changeset;
    class changelog;

    class contains_model: public edge
    {
    public:
      typedef relational::changelog changelog_type;
      typedef relational::model model_type;

      changelog_type&
      changelog () const {return *changelog_;}

      model_type&
      model () const {return *model_;}

    public:
      void
      set_left_node (changelog_type& n) {changelog_ = &n;}

      void
      set_right_node (model_type& n) {model_ = &n;}

    protected:
      changelog_type* changelog_;
      model_type* model_;
    };

    class contains_changeset: public edge
    {
    public:
      typedef relational::changelog changelog_type;
      typedef relational::changeset changeset_type;

      changelog_type&
      changelog () const {return *changelog_;}

      changeset_type&
      changeset () const {return *changeset_;}

    public:
      void
      set_left_node (changelog_type& n) {changelog_ = &n;}

      void
      set_right_node (changeset_type& n) {changeset_ = &n;}

    protected:
      changelog_type* changelog_;
      changeset_type* changeset_;
    };

    class changelog: public graph, public node
    {
      typedef std::vector<contains_changeset*> contains_changeset_list;

    public:
      typedef relational::model model_type;
      typedef relational::contains_model contains_model_type;

      model_type&
      model () const {return contains_model_->model ();}

      contains_model_type&
      contains_model () const {return *contains_model_;}

    public:
      typedef
      pointer_iterator<contains_changeset_list::const_iterator>
      contains_changeset_iterator;

      contains_changeset const&
      contains_changeset_back () const
      {
        return *contains_changeset_.back ();
      }

      contains_changeset const&
      contains_changeset_at (contains_changeset_list::size_type i) const
      {
        return *contains_changeset_[i];
      }

      contains_changeset_iterator
      contains_changeset_begin () const
      {
        return contains_changeset_.begin ();
      }

      contains_changeset_iterator
      contains_changeset_end () const
      {
        return contains_changeset_.end ();
      }

      contains_changeset_list::size_type
      contains_changeset_size () const
      {
        return contains_changeset_.size ();
      }

      bool
      contains_changeset_empty () const
      {
        return contains_changeset_.empty ();
      }

    public:
      string const&
      database () const {return database_;}

      string const&
      schema_name () const {return schema_name_;}

    public:
      changelog (string const& db, string const& sn)
          : database_ (db), schema_name_ (sn), contains_model_ (0) {}
      changelog (xml::parser&);

      void
      add_edge_left (contains_model_type& e)
      {
        assert (contains_model_ == 0);
        contains_model_ = &e;
      }

      void
      add_edge_left (contains_changeset& e)
      {
        contains_changeset_.push_back (&e);
      }

      virtual string
      kind () const {return "changelog";}

      virtual void
      serialize (xml::serializer&) const;

    private:
      changelog (changelog const&);
      changelog& operator= (changelog const&);

    protected:
      string database_;
      string schema_name_;
      contains_model_type* contains_model_;
      contains_changeset_list contains_changeset_;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_CHANGELOG_HXX
