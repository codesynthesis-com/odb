// file      : odb/boost/details/export.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DETAILS_EXPORT_HXX
#define ODB_BOOST_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

#include <odb/boost/details/config.hxx> // LIBODB_BOOST_STATIC if ODB_COMPILER

// Normally we don't export class templates (but do complete specializations),
// inline functions, and classes with only inline member functions. Exporting
// classes that inherit from non-exported/imported bases (e.g., std::string)
// will end up badly. The only known workarounds are to not inherit or to not
// export. Also, MinGW GCC doesn't like seeing non-exported function being
// used before their inline definition. The workaround is to reorder code. In
// the end it's all trial and error.

#if defined(LIBODB_BOOST_STATIC)         // Using static.
#  define LIBODB_BOOST_EXPORT
#elif defined(LIBODB_BOOST_STATIC_BUILD) // Building static.
#  define LIBODB_BOOST_EXPORT
#elif defined(LIBODB_BOOST_SHARED)       // Using shared.
#  ifdef _WIN32
#    define LIBODB_BOOST_EXPORT __declspec(dllimport)
#  else
#    define LIBODB_BOOST_EXPORT
#  endif
#elif defined(LIBODB_BOOST_SHARED_BUILD) // Building shared.
#  ifdef _WIN32
#    define LIBODB_BOOST_EXPORT __declspec(dllexport)
#  else
#    define LIBODB_BOOST_EXPORT
#  endif
#else
// If none of the above macros are defined, then we assume we are being used
// by some third-party build system that cannot/doesn't signal the library
// type. Note that this fallback works for both static and shared but in case
// of shared will be sub-optimal compared to having dllimport.
//
#  define LIBODB_BOOST_EXPORT            // Using static or shared.
#endif

#include <odb/post.hxx>

#endif // ODB_BOOST_DETAILS_EXPORT_HXX
