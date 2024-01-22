// file      : odb/semantics/relational/deferrable.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_DEFERRABLE_HXX
#define ODB_SEMANTICS_RELATIONAL_DEFERRABLE_HXX

#include <string>
#include <iosfwd>

namespace semantics
{
  namespace relational
  {
    struct deferrable
    {
      enum value
      {
        not_deferrable,
        immediate,
        deferred
      };

      deferrable (value v = value (0)) : v_ (v) {}
      operator value () const {return v_;}

      std::string
      string () const;

    private:
      value v_;
    };

    std::ostream&
    operator<< (std::ostream&, deferrable const&);

    std::istream&
    operator>> (std::istream&, deferrable&);
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_DEFERRABLE_HXX
