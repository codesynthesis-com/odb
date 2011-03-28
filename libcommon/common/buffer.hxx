// file      : libcommon/common/buffer.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_BUFFER_HXX
#define LIBCOMMON_COMMON_BUFFER_HXX

#include <cstddef> // std::size_t
#include <cstring> // std::{memcmp,memcpy}

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

#endif // LIBCOMMON_COMMON_BUFFER_HXX
