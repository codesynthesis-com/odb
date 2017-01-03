/* file      : odb/sqlite/details/build2/config.h
 * copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Static configuration file for the build2 build. The installed case
   (when LIBODB_SQLITE_BUILD2 is not necessarily defined) is the only
   reason we have it. */

#ifndef ODB_SQLITE_DETAILS_CONFIG_H
#define ODB_SQLITE_DETAILS_CONFIG_H

/* Define LIBODB_SQLITE_BUILD2 for the installed case. */
#ifndef LIBODB_SQLITE_BUILD2
#  define LIBODB_SQLITE_BUILD2
#endif

#endif /* ODB_SQLITE_DETAILS_CONFIG_H */
