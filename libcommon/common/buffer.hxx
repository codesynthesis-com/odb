// file      : libcommon/common/buffer.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_BUFFER_HXX
#define LIBCOMMON_COMMON_BUFFER_HXX

#include <new>
#include <cstddef> // std::size_t
#include <cstring> // std::{memcmp,memcpy}

struct basic_buffer_base
{
  ~basic_buffer_base ()
  {
    operator delete (data_);
  }

  basic_buffer_base ()
      : data_ (0), size_ (0)
  {
  }

  basic_buffer_base (const void* data, std::size_t size)
      : data_ (0), size_ (size)
  {
    data_ = operator new (size_);
    std::memcpy (data_, data, size_);
  }

  basic_buffer_base (const basic_buffer_base& y)
      : data_ (0), size_ (0)
  {
    assign (y.data_, y.size_);
  }

  basic_buffer_base&
  operator= (const basic_buffer_base& y)
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
      void *p (operator new (size));
      operator delete (data_);
      data_ = p;
    }

    std::memcpy (data_, data, size);
    size_ = size;
  }

  std::size_t
  size () const
  {
    return size_;
  }

  bool
  operator== (const basic_buffer_base& y) const
  {
    return size_ == y.size_ && std::memcmp (data_, y.data_, size_) == 0;
  }

protected:
  void* data_;
  std::size_t size_;
};

template <typename T>
struct basic_buffer: basic_buffer_base
{
  basic_buffer ()
  {
  }

  basic_buffer (const T* data, std::size_t size)
      : basic_buffer_base (data, size)
  {
  }

  T*
  data ()
  {
    return static_cast<T*> (data_);
  }

  const T*
  data () const
  {
    return static_cast<const T*> (data_);
  }
};

typedef basic_buffer<char> buffer;
typedef basic_buffer<unsigned char> ubuffer;

#endif // LIBCOMMON_COMMON_BUFFER_HXX
