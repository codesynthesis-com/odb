// file      : odb/sqlite/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_DETAILS_CONFIG_HXX
#define ODB_SQLITE_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  error libodb-sqlite header included in odb-compiled header
#elif !defined(LIBODB_SQLITE_BUILD2)
#  ifdef _MSC_VER
#    include <odb/sqlite/details/config-vc.h>
#  else
#    include <odb/sqlite/details/config.h>
#  endif
#endif

// LIBODB_SQLITE_BUILD2 macro can be defined either by the buildfile or by the
// included odb/sqlite/details/config*.h (see above).
//
#ifdef LIBODB_SQLITE_BUILD2
#  include <sqlite3.h>

#  if SQLITE_VERSION_NUMBER >= 3006012
#    define LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY 1
#  endif
#  define LIBODB_SQLITE_HAVE_COLUMN_METADATA 1
#endif

// no post

#endif // ODB_SQLITE_DETAILS_CONFIG_HXX
