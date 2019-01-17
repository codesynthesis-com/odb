// file      : odb/boost/details/export.hxx
// copyright : Copyright (c) 2005-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DETAILS_EXPORT_HXX
#define ODB_BOOST_DETAILS_EXPORT_HXX

#include <odb/pre.hxx>

#include <odb/boost/details/config.hxx>

#ifdef LIBODB_BOOST_STATIC_LIB
#  define LIBODB_BOOST_EXPORT
#else
#  ifdef _WIN32
#    ifdef _MSC_VER
#      ifdef LIBODB_BOOST_DYNAMIC_LIB
#        define LIBODB_BOOST_EXPORT __declspec(dllexport)
#      else
#        define LIBODB_BOOST_EXPORT __declspec(dllimport)
#      endif
#    else
#      ifdef LIBODB_BOOST_DYNAMIC_LIB
#        ifdef DLL_EXPORT
#          define LIBODB_BOOST_EXPORT __declspec(dllexport)
#        else
#          define LIBODB_BOOST_EXPORT
#        endif
#      else
#        define LIBODB_BOOST_EXPORT __declspec(dllimport)
#      endif
#    endif
#  else
#    define LIBODB_BOOST_EXPORT
#  endif
#endif

#include <odb/post.hxx>

#endif // ODB_BOOST_DETAILS_EXPORT_HXX
