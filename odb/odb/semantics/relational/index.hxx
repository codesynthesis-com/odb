// file      : odb/semantics/relational/index.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_INDEX_HXX
#define ODB_SEMANTICS_RELATIONAL_INDEX_HXX

#include <odb/semantics/relational/elements.hxx>
#include <odb/semantics/relational/key.hxx>

namespace semantics
{
  namespace relational
  {
    // Note that in our model indexes are defined in the table scope.
    //
    class index: public key
    {
    public:
      string const&
      type () const {return type_;}

      string const&
      method () const {return method_;}

      string const&
      options () const {return options_;}

    public:
      index (string const& id,
             string const& t = string (),
             string const& m = string (),
             string const& o = string ())
          : key (id), type_ (t), method_ (m), options_ (o) {}
      index (index const&, uscope&, graph&);
      index (xml::parser&, uscope&, graph&);

      virtual index&
      clone (uscope&, graph&) const;

      virtual string
      kind () const
      {
        return "index";
      }

      virtual void
      serialize (xml::serializer&) const;

    protected:
      void
      serialize_attributes (xml::serializer&) const;

    private:
      string type_;    // E.g., "UNIQUE", etc.
      string method_;  // E.g., "BTREE", etc.
      string options_; // Database-specific index options.
    };

    class add_index: public index
    {
    public:
      add_index (string const& id,
                 string const& t = string (),
                 string const& m = string (),
                 string const& o = string ())
          : index (id, t, m, o) {}
      add_index (index const& i, uscope& s, graph& g): index (i, s, g) {}
      add_index (xml::parser& p, uscope& s, graph& g): index (p, s, g) {}

      virtual add_index&
      clone (uscope&, graph&) const;

      virtual string
      kind () const {return "add index";}

      virtual void
      serialize (xml::serializer&) const;
    };

    class drop_index: public unameable
    {
    public:
      drop_index (string const& id): unameable (id) {}
      drop_index (drop_index const& di, uscope&, graph& g)
          : unameable (di, g) {}
      drop_index (xml::parser&, uscope&, graph&);

      virtual drop_index&
      clone (uscope&, graph&) const;

      virtual string
      kind () const {return "drop index";}

      virtual void
      serialize (xml::serializer&) const;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_INDEX_HXX
