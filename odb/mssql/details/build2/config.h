/* file      : odb/mssql/details/build2/config.h
 * license   : ODB NCUEL; see accompanying LICENSE file
 */

/* Static configuration file for the build2 build. The installed case
   (when LIBODB_MSSQL_BUILD2 is not necessarily defined) is the only
   reason we have it. */

#ifndef ODB_MSSQL_DETAILS_CONFIG_H
#define ODB_MSSQL_DETAILS_CONFIG_H

/* Define LIBODB_MSSQL_BUILD2 for the installed case. */
#ifndef LIBODB_MSSQL_BUILD2
#  define LIBODB_MSSQL_BUILD2
#endif

#endif /* ODB_MSSQL_DETAILS_CONFIG_H */
