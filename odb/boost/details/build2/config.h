/* file      : odb/boost/details/build2/config.h
 * copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Static configuration file for the build2 build. The installed case
   (when LIBODB_BOOST_BUILD2 is not necessarily defined) is the only
   reason we have it. */

#ifndef ODB_BOOST_DETAILS_CONFIG_H
#define ODB_BOOST_DETAILS_CONFIG_H

/* Define LIBODB_BOOST_BUILD2 for the installed case. */
#ifndef LIBODB_BOOST_BUILD2
#  define LIBODB_BOOST_BUILD2
#endif

#endif /* ODB_BOOST_DETAILS_CONFIG_H */
