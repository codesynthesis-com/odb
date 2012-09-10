// file      : sqlite/types/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <vector>
#include <memory>  // std::auto_ptr

#include <odb/core.hxx>

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

  #pragma db type("BOOL")
  bool bool_;

  #pragma db type("INTEGER")
  int integer_;

  #pragma db type("REAL")
  double real_;

  #pragma db type("REAL")
  double nan_;

  #pragma db type("TEXT")
  std::string text_;

#ifdef _WIN32
  std::string wtext_;
#endif

  #pragma db type("BLOB")
  std::vector<char> blob_;

  // Test NULL value.
  //
  #pragma db type("TEXT") null
  string_ptr null_;

  bool
  operator== (const object& y) const
  {
    return id_ == y.id_
      && bool_ == y.bool_
      && integer_ == y.integer_
      && real_ == y.real_
      && nan_ != nan_
      && text_ == y.text_
#ifdef _WIN32
      && wtext_ == y.wtext_
#endif
      && blob_ == y.blob_
      && ((null_.get () == 0 && y.null_.get () == 0) || *null_ == *y.null_);
  }
};

#endif // TEST_HXX
