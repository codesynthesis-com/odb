// file      : odb/pgsql/details/export.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DETAILS_EXPORT_HXX
#define ODB_PGSQL_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

#include <odb/pgsql/details/config.hxx>

#ifdef LIBODB_PGSQL_STATIC_LIB
#  define LIBODB_PGSQL_EXPORT
#else
#  ifdef _WIN32
#    ifdef _MSC_VER
#      ifdef LIBODB_PGSQL_DYNAMIC_LIB
#        define LIBODB_PGSQL_EXPORT __declspec(dllexport)
#      else
#        define LIBODB_PGSQL_EXPORT __declspec(dllimport)
#      endif
#    else
#      ifdef LIBODB_PGSQL_DYNAMIC_LIB
#        ifdef DLL_EXPORT
#          define LIBODB_PGSQL_EXPORT __declspec(dllexport)
#        else
#          define LIBODB_PGSQL_EXPORT
#        endif
#      else
#        define LIBODB_PGSQL_EXPORT __declspec(dllimport)
#      endif
#    endif
#  else
#    define LIBODB_PGSQL_EXPORT
#  endif
#endif

#include <odb/post.hxx>

#endif // ODB_PGSQL_DETAILS_EXPORT_HXX
