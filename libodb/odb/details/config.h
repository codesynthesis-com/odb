/* file      : odb/details/config.h
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Static configuration file for build2 build.
 *
 * Note that currently we only support ODB_THREADS_NONE and ODB_THREADS_CXX11
 * but could also support the _POSIX and _WIN32 variants with a bit of effort.
 */

#ifndef ODB_DETAILS_CONFIG_H
#define ODB_DETAILS_CONFIG_H

#ifndef ODB_THREADS_NONE
#  define ODB_THREADS_CXX11
#endif

#endif /* ODB_DETAILS_CONFIG_H */
