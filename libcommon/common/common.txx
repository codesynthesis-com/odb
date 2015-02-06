// file      : libcommon/common/common.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// We have to use this helper function instead of just checking which
// database is used because the DATABASE_* macro may not be defined
// in a project that includes this header.
//
LIBCOMMON_EXPORT bool
size_available ();

template <typename T>
std::size_t
size (odb::result<T> r)
{
  if (size_available ())
    return r.size ();
  else
  {
    std::size_t n (0);
    for (typename odb::result<T>::iterator i (r.begin ()); i != r.end (); ++i)
      n++;
    return n;
  }
}
