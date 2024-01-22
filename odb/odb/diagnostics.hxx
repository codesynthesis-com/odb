// file      : odb/diagnostics.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_DIAGNOSTICS_HXX
#define ODB_DIAGNOSTICS_HXX

#include <odb/gcc-fwd.hxx>

#include <string>
#include <cstddef>
#include <iostream>

#include <libcutl/fs/path.hxx>

#include <odb/location.hxx>

using std::endl;

std::ostream&
error (cutl::fs::path const&, std::size_t line, std::size_t clmn);

std::ostream&
warn (cutl::fs::path const&, std::size_t line, std::size_t clmn);

std::ostream&
info (cutl::fs::path const&, std::size_t line, std::size_t clmn);

inline std::ostream&
error (location const& l)
{
  return error (l.file, l.line, l.column);
}

inline std::ostream&
warn (location const&l)
{
  return warn (l.file, l.line, l.column);
}

inline std::ostream&
info (location const&l)
{
  return info (l.file, l.line, l.column);
}

std::ostream&
error (location_t);

std::ostream&
warn (location_t);

std::ostream&
info (location_t);

//
//
class cxx_lexer;

std::ostream&
error (cxx_lexer&);

std::ostream&
warn (cxx_lexer&);

std::ostream&
info (cxx_lexer&);

// Location as a string in the "<file>:<line>:<column>" format.
//
std::string
location_string (cutl::fs::path const&,
                 std::size_t line,
                 std::size_t clmn,
                 bool leaf = false);

inline std::string
location_string (location const& l, bool leaf = false)
{
  return location_string (l.file, l.line, l.column, leaf);
}

std::string
location_string (location_t, bool leaf = false);

// location_t macro wrappers.
//
cutl::fs::path
location_file (location_t);

std::size_t
location_line (location_t);

std::size_t
location_column (location_t);

#endif // ODB_DIAGNOSTICS_HXX
