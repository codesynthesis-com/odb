// file      : libcommon/common/export.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_EXPORT_HXX
#define LIBCOMMON_COMMON_EXPORT_HXX

#include <common/config.hxx>

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
