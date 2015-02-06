// file      : odb/qt/details/export.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DETAILS_EXPORT_HXX
#define ODB_QT_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

#include <odb/qt/details/config.hxx>

#ifdef LIBODB_QT_STATIC_LIB
#  define LIBODB_QT_EXPORT
#else
#  ifdef _WIN32
#    ifdef _MSC_VER
#      ifdef LIBODB_QT_DYNAMIC_LIB
#        define LIBODB_QT_EXPORT __declspec(dllexport)
#      else
#        define LIBODB_QT_EXPORT __declspec(dllimport)
#      endif
#    else
#      ifdef LIBODB_QT_DYNAMIC_LIB
#        ifdef DLL_EXPORT
#          define LIBODB_QT_EXPORT __declspec(dllexport)
#        else
#          define LIBODB_QT_EXPORT
#        endif
#      else
#        define LIBODB_QT_EXPORT __declspec(dllimport)
#      endif
#    endif
#  else
#    define LIBODB_QT_EXPORT
#  endif
#endif

#include <odb/post.hxx>

#endif // ODB_QT_DETAILS_EXPORT_HXX
