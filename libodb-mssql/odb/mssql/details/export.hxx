// file      : odb/mssql/details/export.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_DETAILS_EXPORT_HXX
#define ODB_MSSQL_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

// Note: do this check directly instead of including config.hxx.
//
#ifdef ODB_COMPILER
#  error libodb-mssql header included in odb-compiled header
#endif

// Normally we don't export class templates (but do complete specializations),
// inline functions, and classes with only inline member functions. Exporting
// classes that inherit from non-exported/imported bases (e.g., std::string)
// will end up badly. The only known workarounds are to not inherit or to not
// export. Also, MinGW GCC doesn't like seeing non-exported function being
// used before their inline definition. The workaround is to reorder code. In
// the end it's all trial and error.

#if defined(LIBODB_MSSQL_STATIC)         // Using static.
#  define LIBODB_MSSQL_EXPORT
#elif defined(LIBODB_MSSQL_STATIC_BUILD) // Building static.
#  define LIBODB_MSSQL_EXPORT
#elif defined(LIBODB_MSSQL_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBODB_MSSQL_EXPORT __declspec(dllimport)
#  else
#    define LIBODB_MSSQL_EXPORT
#  endif
#elif defined(LIBODB_MSSQL_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBODB_MSSQL_EXPORT __declspec(dllexport)
#  else
#    define LIBODB_MSSQL_EXPORT
#  endif
#else
// If none of the above macros are defined, then we assume we are being used
// by some third-party build system that cannot/doesn't signal the library
// type. Note that this fallback works for both static and shared but in case
// of shared will be sub-optimal compared to having dllimport.
//
#  define LIBODB_MSSQL_EXPORT            // Using static or shared.
#endif

#include <odb/post.hxx>

#endif // ODB_MSSQL_DETAILS_EXPORT_HXX
