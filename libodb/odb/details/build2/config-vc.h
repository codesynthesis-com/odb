/* file      : odb/details/build2/config-vc.h
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Configuration file for Windows/VC++ for the build2 build. */

#ifndef ODB_DETAILS_CONFIG_VC_H
#define ODB_DETAILS_CONFIG_VC_H

/* Define LIBODB_BUILD2 for the installed case. */
#ifndef LIBODB_BUILD2
#  define LIBODB_BUILD2
#endif

#ifndef ODB_THREADS_NONE
#  if _MSC_VER >= 1900
#    define ODB_THREADS_CXX11
#  else
#    error Unsupoprted MSVC version (no thread_local)
#  endif
#endif

#endif /* ODB_DETAILS_CONFIG_VC_H */
