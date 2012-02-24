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
#if _MSC_VER >= 1600
#  define HAVE_CXX11
#endif

#endif /* LIBCOMMON_COMMON_CONFIG_VC_H */
