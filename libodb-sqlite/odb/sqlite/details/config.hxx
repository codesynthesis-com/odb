// file      : odb/sqlite/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_DETAILS_CONFIG_HXX
#define ODB_SQLITE_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  error libodb-sqlite header included in odb-compiled header
#endif

#include <sqlite3.h>

#if SQLITE_VERSION_NUMBER >= 3006012 && !defined(ODB_THREADS_NONE)
#  define LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY 1
#endif

#define LIBODB_SQLITE_HAVE_COLUMN_METADATA 1

// no post

#endif // ODB_SQLITE_DETAILS_CONFIG_HXX
