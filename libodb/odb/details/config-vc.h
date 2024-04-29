/* file      : odb/details/config-vc.h
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Configuration file for Windows/VC++ for the build2 build.
 *
 * Note that currently we only support ODB_THREADS_NONE and ODB_THREADS_CXX11
 * but could also support the _WIN32 variant with a bit of effort.
 *
 */

#ifndef ODB_DETAILS_CONFIG_VC_H
#define ODB_DETAILS_CONFIG_VC_H

#ifndef ODB_THREADS_NONE
#  if _MSC_VER >= 1900
#    define ODB_THREADS_CXX11
#  else
#    error Unsupoprted MSVC version (no thread_local)
#  endif
#endif

#endif /* ODB_DETAILS_CONFIG_VC_H */
