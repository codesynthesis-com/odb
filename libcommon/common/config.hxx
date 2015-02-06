// file      : libcommon/common/config.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_CONFIG_HXX
#define LIBCOMMON_COMMON_CONFIG_HXX

#ifdef HAVE_CONFIG_VC_H
#  include <common/config-vc.h>
#else
#  include <common/config.h>

// GCC supports strongly typed enums from 4.4 (forward -- 4.6),
// Clang -- 2.9 (3.1).
//
#  ifdef HAVE_CXX11
#    define HAVE_CXX11_ENUM
#  endif
#endif

#endif // LIBCOMMON_COMMON_CONFIG_HXX
