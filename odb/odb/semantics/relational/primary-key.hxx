// file      : odb/semantics/relational/primary-key.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_PRIMARY_KEY_HXX
#define ODB_SEMANTICS_RELATIONAL_PRIMARY_KEY_HXX

#include <map>

#include <odb/semantics/relational/elements.hxx>
#include <odb/semantics/relational/key.hxx>

namespace semantics
{
  namespace relational
  {
    class primary_key: public key
    {
    public:
      bool
      auto_ () const {return auto__;}

      // Extra information.
      //
    public:
      typedef std::map<string, string> extra_map;

      extra_map&
      extra () {return extra_map_;}

      extra_map const&
      extra () const {return extra_map_;}

    public:
      primary_key (bool auto_)
          : key (""),      // Primary key has the implicit empty id.
            auto__ (auto_)
      {
      }

      primary_key (primary_key const&, uscope&, graph&);
      primary_key (xml::parser&, uscope&, graph&);

      virtual primary_key&
      clone (uscope&, graph&) const;

      virtual string
      kind () const
      {
        return "primary key";
      }

      virtual void
      serialize (xml::serializer&) const;

    private:
      bool auto__;
      extra_map extra_map_;
    };
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_PRIMARY_KEY_HXX
