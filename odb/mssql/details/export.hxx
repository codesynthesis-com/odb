// file      : odb/mssql/details/export.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_DETAILS_EXPORT_HXX
#define ODB_MSSQL_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

#include <odb/mssql/details/config.hxx>

#ifdef LIBODB_MSSQL_STATIC_LIB
#  define LIBODB_MSSQL_EXPORT
#else
#  ifdef _WIN32
#    ifdef _MSC_VER
#      ifdef LIBODB_MSSQL_DYNAMIC_LIB
#        define LIBODB_MSSQL_EXPORT __declspec(dllexport)
#      else
#        define LIBODB_MSSQL_EXPORT __declspec(dllimport)
#      endif
#    else
#      ifdef LIBODB_MSSQL_DYNAMIC_LIB
#        ifdef DLL_EXPORT
#          define LIBODB_MSSQL_EXPORT __declspec(dllexport)
#        else
#          define LIBODB_MSSQL_EXPORT
#        endif
#      else
#        define LIBODB_MSSQL_EXPORT __declspec(dllimport)
#      endif
#    endif
#  else
#    define LIBODB_MSSQL_EXPORT
#  endif
#endif

#include <odb/post.hxx>

#endif // ODB_MSSQL_DETAILS_EXPORT_HXX
