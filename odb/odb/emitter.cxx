// file      : odb/emitter.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/context.hxx>
#include <odb/emitter.hxx>

using namespace std;

void emitter::
pre ()
{
}

void emitter::
post ()
{
}

int emitter_ostream::streambuf::
sync ()
{
  string s (str ());

  // Get rid of the trailing newline if any.
  //
  if (string::size_type n = s.size ())
  {
    if (s[n - 1] == '\n')
      s.resize (n - 1);
  }

  // Temporary restore output diversion.
  //
  bool r (false);
  context& ctx (context::current ());

  if (ctx.os.rdbuf () == this)
  {
    ctx.restore ();
    r = true;
  }

  e_.line (s);

  if (r)
    ctx.diverge (this);

  str (string ());
  return 0;
}
