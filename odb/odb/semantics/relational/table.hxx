// file      : odb/semantics/relational/table.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_TABLE_HXX
#define ODB_SEMANTICS_RELATIONAL_TABLE_HXX

#include <odb/semantics/relational/elements.hxx>

namespace semantics
{
  namespace relational
  {
    class table: public qnameable, public uscope
    {
    public:
      virtual string const&
      options () const {return options_;}

      virtual void
      options (string const& o) {options_ = o;}

      // Extra information.
      //
    public:
      typedef std::map<string, string> extra_map;

      extra_map&
      extra () {return extra_map_;}

      extra_map const&
      extra () const {return extra_map_;}

    public:
      table (string const& id): qnameable (id) {}
      table (table const&, qscope&, graph&, bool base = false);
      table (xml::parser&, qscope&, graph&, bool base = false);

      virtual table&
      clone (qscope&, graph&) const;

      virtual string
      kind () const {return "table";}

      virtual void
      serialize (xml::serializer&) const;

      // Resolve ambiguity.
      //
      using qnameable::scope;

    protected:
      void
      serialize_attributes (xml::serializer&) const;

    protected:
      string options_;
      extra_map extra_map_;
    };

    class add_table: public table
    {
    public:
      add_table (string const& id): table (id) {}
      add_table (table const& t, qscope& s, graph& g): table (t, s, g) {}
      add_table (xml::parser& p, qscope& s, graph& g): table (p, s, g) {}

      virtual add_table&
      clone (qscope&, graph&) const;

      virtual string
      kind () const {return "add table";}

      virtual void
      serialize (xml::serializer&) const;
    };

    class drop_table: public qnameable
    {
    public:
      drop_table (string const& id): qnameable (id) {}
      drop_table (drop_table const& t, qscope&, graph& g): qnameable (t, g) {}
      drop_table (xml::parser&, qscope&, graph&);

      virtual drop_table&
      clone (qscope&, graph&) const;

      virtual string
      kind () const {return "drop table";}

      virtual void
      serialize (xml::serializer&) const;
    };

    class alter_table: public table
    {
    public:
      alter_table (string const& id): table (id) {}
      alter_table (alter_table const& at, qscope& s, graph& g)
          : table (at, s, g, true) {}
      alter_table (xml::parser& p, qscope& s, graph& g)
          : table (p, s, g, true) {}

      virtual alter_table&
      clone (qscope&, graph&) const;

      virtual string
      kind () const {return "alter table";}

      virtual void
      serialize (xml::serializer&) const;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_TABLE_HXX
