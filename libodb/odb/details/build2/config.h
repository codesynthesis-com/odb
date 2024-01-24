/* file      : odb/details/build2/config.h
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Static configuration file for the build2 build. */

#ifndef ODB_DETAILS_CONFIG_H
#define ODB_DETAILS_CONFIG_H

/* Define LIBODB_BUILD2 for the installed case. */
#ifndef LIBODB_BUILD2
#  define LIBODB_BUILD2
#endif

#ifndef ODB_THREADS_NONE
#  define ODB_THREADS_CXX11
#endif

#endif /* ODB_DETAILS_CONFIG_H */
