// file      : odb/location.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_LOCATION_HXX
#define ODB_LOCATION_HXX

#include <odb/gcc-fwd.hxx>

#include <cstddef>
#include <libcutl/fs/path.hxx>

struct location
{
  location (location_t);
  location (cutl::fs::path const& f, std::size_t l, std::size_t c)
      : file (f), line (l), column (c)
  {
  }

  cutl::fs::path file;
  std::size_t line;
  std::size_t column;
};

#endif // ODB_LOCATION_HXX
