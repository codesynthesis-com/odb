// file      : libcommon/common/export.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_EXPORT_HXX
#define LIBCOMMON_COMMON_EXPORT_HXX

#ifndef _MSC_VER
#  include <common/config.h>
#endif

#ifdef LIBCOMMON_STATIC_LIB
#  define LIBCOMMON_EXPORT
#else
#  ifdef _WIN32
#    ifdef _MSC_VER
#      ifdef LIBCOMMON_DYNAMIC_LIB
#        define LIBCOMMON_EXPORT __declspec(dllexport)
#      else
#        define LIBCOMMON_EXPORT __declspec(dllimport)
#      endif
#    else
#      ifdef LIBCOMMON_DYNAMIC_LIB
#        ifdef DLL_EXPORT
#          define LIBCOMMON_EXPORT __declspec(dllexport)
#        else
#          define LIBCOMMON_EXPORT
#        endif
#      else
#        define LIBCOMMON_EXPORT __declspec(dllimport)
#      endif
#    endif
#  else
#    define LIBCOMMON_EXPORT
#  endif
#endif

#endif // LIBCOMMON_COMMON_EXPORT_HXX
