// file      : odb/details/tls.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_TLS_HXX
#define ODB_DETAILS_TLS_HXX

#include <odb/pre.hxx>

#include <odb/details/config.hxx>

#ifdef ODB_THREADS_NONE

#  define ODB_TLS_POINTER(type) type*
#  define ODB_TLS_OBJECT(type) type

namespace odb
{
  namespace details
  {
    template <typename T>
    inline T&
    tls_get (T& x)
    {
      return x;
    }

    // If early destructions is possible, destroy the object and free
    // any allocated resources.
    //
    template <typename T>
    inline void
    tls_free (T&)
    {
    }

    template <typename T>
    inline T*
    tls_get (T* p)
    {
      return p;
    }

    template <typename T, typename T1>
    inline void
    tls_set (T*& rp, T1* p)
    {
      rp = p;
    }
  }
}

#elif defined(ODB_THREADS_CXX11)

// Apparently Apple's Clang "temporarily disabled" C++11 thread_local until
// they can implement a "fast" version, which reportedly happened in XCode 8.
// So for now we will continue using __thread for this target.
//
#  if defined(__apple_build_version__) && __apple_build_version__ < 8000000
#    define ODB_TLS_POINTER(type) __thread type*
#    define ODB_TLS_OBJECT(type) thread_local type
#  else
#    define ODB_TLS_POINTER(type) thread_local type*
#    define ODB_TLS_OBJECT(type) thread_local type
#  endif

namespace odb
{
  namespace details
  {
    template <typename T>
    inline T&
    tls_get (T& x)
    {
      return x;
    }

    template <typename T>
    inline void
    tls_free (T&)
    {
    }

    template <typename T>
    inline T*
    tls_get (T* p)
    {
      return p;
    }

    template <typename T, typename T1>
    inline void
    tls_set (T*& rp, T1* p)
    {
      rp = p;
    }
  }
}

#elif defined(ODB_THREADS_POSIX)

#  include <odb/details/posix/tls.hxx>

#  ifdef ODB_THREADS_TLS_KEYWORD
#    define ODB_TLS_POINTER(type) __thread type*

namespace odb
{
  namespace details
  {
    template <typename T>
    inline T*
    tls_get (T* p)
    {
      return p;
    }

    template <typename T, typename T1>
    inline void
    tls_set (T*& rp, T1* p)
    {
      rp = p;
    }
  }
}

#  else
#    define ODB_TLS_POINTER(type) tls<type*>
#  endif
#  define ODB_TLS_OBJECT(type) tls<type>

#elif defined(ODB_THREADS_WIN32)

#  include <odb/details/win32/tls.hxx>

#  ifdef ODB_THREADS_TLS_DECLSPEC
#    define ODB_TLS_POINTER(type) __declspec(thread) type*

namespace odb
{
  namespace details
  {
    template <typename T>
    inline T*
    tls_get (T* p)
    {
      return p;
    }

    template <typename T, typename T1>
    inline void
    tls_set (T*& rp, T1* p)
    {
      rp = p;
    }
  }
}

#  else
#    define ODB_TLS_POINTER(type) tls<type*>
#  endif
#  define ODB_TLS_OBJECT(type) tls<type>
#else
# error unknown threading model
#endif

#include <odb/post.hxx>

#endif // ODB_DETAILS_TLS_HXX
