/* file      : libcommon/common/config-vc.h
 * copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
 * license   : GNU GPL v2; see accompanying LICENSE file
 */

/* Configuration file for Windows/VC++. */

#ifndef LIBCOMMON_COMMON_CONFIG_VC_H
#define LIBCOMMON_COMMON_CONFIG_VC_H

#define HAVE_TR1_MEMORY

/* VC++10 has C++11 always enabled.
 */
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || \
    (defined(ODB_MSC_VER) && ODB_MSC_VER >= 1600)
#  define HAVE_CXX11
#endif

#endif /* LIBCOMMON_COMMON_CONFIG_VC_H */
