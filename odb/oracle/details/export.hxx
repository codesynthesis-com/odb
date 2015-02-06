// file      : odb/oracle/details/export.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_DETAILS_EXPORT_HXX
#define ODB_ORACLE_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

#include <odb/oracle/details/config.hxx>

#ifdef LIBODB_ORACLE_STATIC_LIB
#  define LIBODB_ORACLE_EXPORT
#else
#  ifdef _WIN32
#    ifdef _MSC_VER
#      ifdef LIBODB_ORACLE_DYNAMIC_LIB
#        define LIBODB_ORACLE_EXPORT __declspec(dllexport)
#      else
#        define LIBODB_ORACLE_EXPORT __declspec(dllimport)
#      endif
#    else
#      ifdef LIBODB_ORACLE_DYNAMIC_LIB
#        ifdef DLL_EXPORT
#          define LIBODB_ORACLE_EXPORT __declspec(dllexport)
#        else
#          define LIBODB_ORACLE_EXPORT
#        endif
#      else
#        define LIBODB_ORACLE_EXPORT __declspec(dllimport)
#      endif
#    endif
#  else
#    define LIBODB_ORACLE_EXPORT
#  endif
#endif

#include <odb/post.hxx>

#endif // ODB_ORACLE_DETAILS_EXPORT_HXX
