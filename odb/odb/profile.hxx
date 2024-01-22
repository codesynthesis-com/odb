// file      : odb/profile.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_PROFILE_HXX
#define ODB_PROFILE_HXX

#include <set>
#include <vector>
#include <string>

#include <libcutl/fs/path.hxx>

#include <odb/option-types.hxx>

struct profile_data
{
  typedef cutl::fs::path path;
  typedef std::vector<path> paths;

  profile_data (paths const& p, database d, char const* n)
      : search_paths (p), db (d), name (n)
  {
  }

  paths const& search_paths;
  database db;
  char const* name;
  std::set<path> loaded;
};

struct profile_failure {};

std::string
profile_search (char const* profile, void* arg);

std::string
profile_search_ignore (char const* profile, void* arg);

#endif // ODB_PROFILE_HXX
