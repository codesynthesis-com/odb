// file      : odb/instance.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <sstream>
#include <cstdlib>  // abort
#include <cxxabi.h> // abi::__cxa_demangle

#include <odb/instance.hxx>

using namespace std;

struct demangled_name
{
  demangled_name (): s (0), n (0) {}
  ~demangled_name () {free (s);}
  char* s;
  size_t n;
};

static demangled_name name_;

string entry_base::
name (type_info const& ti)
{
  char*& s (name_.s);

  {
    int r;
    s = abi::__cxa_demangle (ti.name (), s, &name_.n, &r);

    if (r != 0)
      abort (); // We are in static initialization, so this is fatal.
  }

  string str (s), r;

  // Get the first component. It can be a database kind or name.
  //
  string::size_type p (str.find ("::"));

  if (p == string::npos)
    abort (); // Derived type should be in a namespace.

  string n (str, 0, p);

  // See if it is one of the known kinds.
  //
  if (n == "relational")
  {
    r = n;
    p = str.find ("::", 12); // 12 for "relational::"
    n.assign (str, 12, p == string::npos ? p : p - 12);
  }

  // See if it is one of the known databases.
  //
  database db;
  istringstream is (n);
  if (!(is >> db))
  {
    if (r.empty ())
      abort (); // Has to have either kind or database.
  }
  else
  {
    if (!r.empty ())
      r += "::";

    r += n;
  }

  return r;
}
