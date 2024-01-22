// file      : odb/diagnostics.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <sstream>

#include <odb/cxx-lexer.hxx>
#include <odb/diagnostics.hxx>

using namespace std;
using cutl::fs::path;

std::ostream&
error (path const& p, size_t line, size_t clmn)
{
  //@@ We only need to do this if we are still parsing (i.e.,
  //   pragma parsing). Is there a way to detect this?
  //
  errorcount++;

  cerr << p << ':' << line << ':' << clmn << ": error: ";
  return cerr;
}

std::ostream&
warn (path const& p, size_t line, size_t clmn)
{
  warningcount++;

  cerr << p << ':' << line << ':' << clmn << ": warning: ";
  return cerr;
}

std::ostream&
info (path const& p, size_t line, size_t clmn)
{
  cerr << p << ':' << line << ':' << clmn << ": info: ";
  return cerr;
}

std::ostream&
error (location_t loc)
{
  errorcount++;
  cerr << LOCATION_FILE (loc) << ':'
       << LOCATION_LINE (loc) << ':'
       << LOCATION_COLUMN (loc) << ':'
       << " error: ";
  return cerr;
}

std::ostream&
warn (location_t loc)
{
  warningcount++;
  cerr << LOCATION_FILE (loc) << ':'
       << LOCATION_LINE (loc) << ':'
       << LOCATION_COLUMN (loc) << ':'
       << " warning: ";
  return cerr;
}

std::ostream&
info (location_t loc)
{
  cerr << LOCATION_FILE (loc) << ':'
       << LOCATION_LINE (loc) << ':'
       << LOCATION_COLUMN (loc) << ':'
       << " info: ";
  return cerr;
}

std::ostream&
error (cxx_lexer& l)
{
  return error (l.location ());
}

std::ostream&
warn (cxx_lexer& l)
{
  return warn (l.location ());
}

std::ostream&
info (cxx_lexer& l)
{
  return info (l.location ());
}

std::string
location_string (path const& p, size_t line, size_t clmn, bool leaf)
{
  ostringstream ostr;

  if (leaf)
    ostr << p.leaf ();
  else
    ostr << p;

  ostr << ':' << line << ':' << clmn;
  return ostr.str ();
}

std::string
location_string (location_t loc, bool leaf)
{
  ostringstream ostr;

  if (leaf)
    ostr << path (LOCATION_FILE (loc)).leaf ();
  else
    ostr << LOCATION_FILE (loc);

  ostr << ':' << LOCATION_LINE (loc) << ':' << LOCATION_COLUMN (loc);
  return ostr.str ();
}

path
location_file (location_t loc)
{
  return path (LOCATION_FILE (loc));
}

size_t
location_line (location_t loc)
{
  return LOCATION_LINE (loc);
}

size_t
location_column (location_t loc)
{
  return LOCATION_COLUMN (loc);
}
