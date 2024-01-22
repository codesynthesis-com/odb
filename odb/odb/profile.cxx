// file      : odb/profile.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <unistd.h>    // stat
#include <sys/types.h> // stat
#include <sys/stat.h>  // stat

#include <iostream>

#include <odb/profile.hxx>

using namespace std;

static bool
exist (profile_data::path const& p)
{
  struct stat info;

  // Just check that the file exist without checking for permissions, etc.
  //
  return stat (p.string ().c_str (), &info) == 0 && S_ISREG (info.st_mode);
}

string
profile_search (char const* prof, void* arg)
{
  typedef profile_data::path path;
  typedef profile_data::paths paths;

  profile_data* pd (static_cast<profile_data*> (arg));
  paths const& ps (pd->search_paths);

  path p (prof), odb ("odb"), r;
  p.normalize (); // Convert '/' to the canonical path separator form.
  path p_db (p);
  p_db += "-";
  p_db += pd->db.string ();
  p += ".options";
  p_db += ".options";

  paths::const_iterator i (ps.begin ()), end (ps.end ());
  for (; i != end; ++i)
  {
    // First check for the database-specific version in the search directory
    // itself and then try the odb/ subdirectory.
    //
    if (exist (r = *i / p_db))
      break;

    if (exist (r = *i / odb / p_db))
      break;

    // Then try the same with the database-independent version.
    //
    if (exist (r = *i / p))
      break;

    if (exist (r = *i / odb / p))
      break;
  }

  if (i == end)
  {
    // Ignore the case where we didn't find the profile and this is the
    // common database.
    //
    if (pd->db == database::common)
      return string ();

    cerr << pd->name << ": error: unable to locate options file for profile '"
         << prof << "'" << endl;
    throw profile_failure ();
  }

  if (pd->loaded.find (r) != pd->loaded.end ())
    return string ();

  pd->loaded.insert (r);
  return r.string ();
}

string
profile_search_ignore (char const*, void*)
{
  return string ();
}
