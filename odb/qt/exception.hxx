// file      : odb/qt/exception.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_EXCEPTION_HXX
#define ODB_QT_EXCEPTION_HXX

#include <odb/pre.hxx>

#include <odb/exceptions.hxx>

#include <odb/qt/details/export.hxx>

namespace odb
{
  namespace qt
  {
    struct LIBODB_QT_EXPORT exception: odb::exception
    {
      virtual const char*
      what () const throw () = 0;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_EXCEPTION_HXX
