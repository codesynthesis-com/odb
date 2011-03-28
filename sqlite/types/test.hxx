// file      : sqlite/types/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <memory>  // std::auto_ptr

#include <odb/core.hxx>

#include <common/buffer.hxx>

typedef std::auto_ptr<std::string> string_ptr;

#pragma db object
struct object
{
  object (unsigned long id)
      : id_ (id)
  {
  }

  object ()
  {
  }

  #pragma db id
  unsigned long id_;

  #pragma db type ("BOOL NOT NULL")
  bool bool_;

  #pragma db type ("INTEGER NOT NULL")
  int integer_;

  #pragma db type ("REAL NOT NULL")
  double real_;

  #pragma db type ("TEXT NOT NULL")
  std::string text_;

  #pragma db type ("BLOB NOT NULL")
  buffer blob_;

  // Test NULL value.
  //
  #pragma db type ("TEXT")
  string_ptr null_;

  bool
  operator== (const object& y) const
  {
    return
      id_ == y.id_ &&
      bool_ == y.bool_ &&
      integer_ == y.integer_ &&
      real_ == y.real_ &&
      text_ == y.text_ &&
      blob_ == y.blob_ &&
      ((null_.get () == 0 && y.null_.get () == 0) || *null_ == *y.null_);
  }
};

#endif // TEST_HXX
