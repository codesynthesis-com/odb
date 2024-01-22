// file      : odb/semantics/relational/name.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <ostream>
#include <istream>

#include <odb/semantics/relational/name.hxx>

using namespace std;

namespace semantics
{
  namespace relational
  {
    string qname::
    string () const
    {
      std::string r;

      bool f (true);
      for (iterator i (begin ()); i < end (); ++i)
      {
        if (i->empty ())
          continue;

        if (f)
          f = false;
        else
          r += '.';

        r += *i;
      }

      return r;
    }

    qname qname::
    from_string (std::string const& s)
    {
      using std::string;

      qname n;

      string::size_type p (string::npos);

      for (size_t i (0); i < s.size (); ++i)
      {
        char c (s[i]);

        if (c == '.')
        {
          if (p == string::npos)
            n.append (string (s, 0, i));
          else
            n.append (string (s, p + 1, i - p - 1));

          p  = i;
        }
      }

      if (p == string::npos)
        n.append (s);
      else
        n.append (string (s, p + 1, string::npos));

      return n;
    }

    ostream&
    operator<< (ostream& os, qname const& n)
    {
      return os << n.string ();
    }

    istream&
    operator>> (istream& is, qname& n)
    {
      string s;
      is >> s;

      if (!is.fail ())
        n = qname::from_string (s);
      else
        n.clear ();

      return is;
    }
  }
}
