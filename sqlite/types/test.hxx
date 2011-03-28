// file      : sqlite/types/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <memory>  // std::auto_ptr
#include <cstddef> // std::size_t
#include <cstring> // std::{memcmp,memcpy}

#include <odb/core.hxx>

struct buffer
{
  ~buffer ()
  {
    delete[] data_;
  }

  buffer ()
      : data_ (0), size_ (0)
  {
  }

  buffer (const void* data, std::size_t size)
      : data_ (0), size_ (size)
  {
    data_ = new char[size_];
    std::memcpy (data_, data, size_);
  }

  buffer (const buffer& y)
      : data_ (0), size_ (0)
  {
    assign (y.data_, y.size_);
  }

  buffer&
  operator= (const buffer& y)
  {
    if (this != &y)
      assign (y.data_, y.size_);

    return *this;
  }

  void
  assign (const void* data, std::size_t size)
  {
    if (size_ < size)
    {
      char* p (new char[size]);
      delete[] data_;
      data_ = p;
    }

    std::memcpy (data_, data, size);
    size_ = size;
  }

  char*
  data ()
  {
    return data_;
  }

  const char*
  data () const
  {
    return data_;
  }

  std::size_t
  size () const
  {
    return size_;
  }

  bool
  operator== (const buffer& y) const
  {
    return size_ == y.size_ && std::memcmp (data_, y.data_, size_) == 0;
  }

private:
  char* data_;
  std::size_t size_;
};

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
