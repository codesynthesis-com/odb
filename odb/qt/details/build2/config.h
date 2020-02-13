/* file      : odb/qt/details/build2/config.h
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Static configuration file for the build2 build. The installed case
   (when LIBODB_QT_BUILD2 is not necessarily defined) is the only
   reason we have it. */

#ifndef ODB_QT_DETAILS_CONFIG_H
#define ODB_QT_DETAILS_CONFIG_H

/* Define LIBODB_QT_BUILD2 for the installed case. */
#ifndef LIBODB_QT_BUILD2
#  define LIBODB_QT_BUILD2
#endif

#endif /* ODB_QT_DETAILS_CONFIG_H */
