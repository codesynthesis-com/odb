// file      : odb/semantics/relational/deferrable.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <ostream>
#include <istream>

#include <odb/semantics/relational/deferrable.hxx>

using namespace std;

namespace semantics
{
  namespace relational
  {
    static const char* deferrable_[] =
    {
      "NOT DEFERRABLE",
      "IMMEDIATE",
      "DEFERRED"
    };

    string deferrable::
    string () const
    {
      return deferrable_[v_];
    }

    ostream&
    operator<< (ostream& os, deferrable const& v)
    {
      return os << v.string ();
    }

    istream&
    operator>> (istream& is, deferrable& v)
    {
      string s;
      is >> s;

      if (!is.fail ())
      {
        if (s == "not_deferrable" || s == "NOT DEFERRABLE")
          v = deferrable::not_deferrable;
        else if (s == "immediate" || s == "IMMEDIATE")
          v = deferrable::immediate;
        else if (s == "deferred" || s == "DEFERRED")
          v = deferrable::deferred;
        else
          is.setstate (istream::failbit);
      }

      return is;
    }
  }
}
