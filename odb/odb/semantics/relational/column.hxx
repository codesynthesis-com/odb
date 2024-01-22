// file      : odb/semantics/relational/column.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_COLUMN_HXX
#define ODB_SEMANTICS_RELATIONAL_COLUMN_HXX

#include <odb/semantics/relational/elements.hxx>
#include <odb/semantics/relational/table.hxx>

namespace semantics
{
  namespace relational
  {
    class contains;

    class column: public unameable
    {
      typedef std::vector<contains*> contained_list;

    public:
      virtual string const&
      type () const {return type_;}

      virtual bool
      null () const {return null_;}

      virtual void
      null (bool n) {null_ = n;}

      virtual string const&
      default_ () const {return default__;}

      virtual void
      default_ (string const& d) {default__ = d;}

      virtual string const&
      options () const {return options_;}

      virtual void
      options (string const& o) {options_ = o;}

    public:
      typedef relational::table table_type;

      table_type&
      table () const {return dynamic_cast<table_type&> (scope ());}

      // Key containment.
      //
    public:
      typedef
      pointer_iterator<contained_list::const_iterator>
      contained_iterator;

      contained_iterator
      contained_begin () const {return contained_.begin ();}

      contained_iterator
      contained_end () const {return contained_.end ();}

    public:
      column (string const& id, string const& type, bool null)
          : unameable (id), type_ (type), null_ (null)
      {
      }

      column (column const&, uscope&, graph&);
      column (xml::parser&, uscope&, graph&);

      virtual column&
      clone (uscope&, graph&) const;

      void
      add_edge_right (contains& e)
      {
        contained_.push_back (&e);
      }

      using unameable::add_edge_right;

      virtual string
      kind () const
      {
        return "column";
      }

      virtual void
      serialize (xml::serializer&) const;

    protected:
      void
      serialize_attributes (xml::serializer&) const;

    protected:
      string type_;
      bool null_;
      string default__;
      string options_;

    private:
      contained_list contained_;
    };

    class add_column: public column
    {
    public:
      add_column (string const& id, string const& type, bool null)
          : column (id, type, null) {}
      add_column (column const& c, uscope& s, graph& g): column (c, s, g) {}
      add_column (xml::parser& p, uscope& s, graph& g): column (p, s, g) {}

      virtual add_column&
      clone (uscope&, graph&) const;

      virtual string
      kind () const {return "add column";}

      virtual void
      serialize (xml::serializer&) const;
    };

    class drop_column: public unameable
    {
    public:
      drop_column (string const& id): unameable (id) {}
      drop_column (drop_column const& c, uscope&, graph& g)
          : unameable (c, g) {}
      drop_column (xml::parser&, uscope&, graph&);

    public:
      typedef relational::table table_type;

      table_type&
      table () const {return dynamic_cast<table_type&> (scope ());}

    public:
      virtual drop_column&
      clone (uscope&, graph&) const;

      virtual string
      kind () const {return "drop column";}

      virtual void
      serialize (xml::serializer&) const;
    };

    class alter_column: public column
    {
    public:
      virtual string const&
      type () const {return base ().type ();}

      bool
      null_altered () const {return null_altered_;}

      virtual bool
      null () const {return null_altered_ ? null_ : base ().null ();}

      virtual void
      null (bool n) {null_ = n; null_altered_ = true;}

      virtual string const&
      default_ () const {return base ().default_ ();}

      virtual string const&
      options () const {return base ().options ();}

    public:
      column&
      base () const {return dynamic_cast<column&> (alters_->base ());}

    public:
      alter_column (string const& id)
          : column (id, "", false), alters_ (0), null_altered_ (false) {}
      alter_column (alter_column const&, uscope&, graph&);
      alter_column (xml::parser&, uscope&, graph&);

      virtual alter_column&
      clone (uscope&, graph&) const;

      virtual string
      kind () const
      {
        return "alter column";
      }

      virtual void
      serialize (xml::serializer&) const;

      virtual void
      add_edge_left (alters& a)
      {
        assert (alters_ == 0);
        alters_ = &a;
      }

    private:
      alters* alters_;

      bool null_altered_;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_COLUMN_HXX
