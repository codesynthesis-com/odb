// file      : odb/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_CONFIG_HXX
#define ODB_DETAILS_CONFIG_HXX

// no pre

// C++11 support.
//
#ifdef _MSC_VER
#  if _MSC_VER >= 1600 // VC++10 and later have C++11 always enabled.
#    define ODB_CXX11
#    define ODB_CXX11_NULLPTR
#    if _MSC_VER >= 1700
#      define ODB_CXX11_ENUM
#      if _MSC_VER >= 1800
#        define ODB_CXX11_DELETED_FUNCTION
#        define ODB_CXX11_EXPLICIT_CONVERSION_OPERATOR
#        define ODB_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGUMENT
#        define ODB_CXX11_VARIADIC_TEMPLATE
#        define ODB_CXX11_INITIALIZER_LIST
#        if _MSC_VER >= 1900
#          define ODB_CXX11_NOEXCEPT
#        endif
#      endif
#    endif
#  endif
#else
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    define ODB_CXX11
#    ifdef __clang__ // Pretends to be a really old __GNUC__ on some platforms.
#      define ODB_CXX11_NULLPTR
#      define ODB_CXX11_NOEXCEPT
#    elif defined(__GNUC__)
#      if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4
#        define ODB_CXX11_NULLPTR
#        define ODB_CXX11_NOEXCEPT
#      endif
#    else
#      define ODB_CXX11_NULLPTR
#      define ODB_CXX11_NOEXCEPT
#    endif
#    define ODB_CXX11_DELETED_FUNCTION
#    define ODB_CXX11_EXPLICIT_CONVERSION_OPERATOR
#    define ODB_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGUMENT
#    define ODB_CXX11_VARIADIC_TEMPLATE
#    define ODB_CXX11_INITIALIZER_LIST
#    define ODB_CXX11_ENUM // GCC 4.4 (forward -- 4.6), Clang 2.9 (3.1).
#  endif
#endif

#ifdef ODB_CXX11_NOEXCEPT
#  define ODB_NOTHROW_NOEXCEPT noexcept
#else
#  define ODB_NOTHROW_NOEXCEPT throw()
#endif

// Once we drop support for C++98, we can probably get rid of config.h except
// for the autotools case by fixing ODB_THREADS_CXX11 (and perhaps supporting
// the ODB_THREADS_NONE case via a "global" (command line) define).
//
#ifdef ODB_COMPILER
#  define ODB_THREADS_NONE
#  define LIBODB_STATIC
#elif defined(_MSC_VER)
#  include <odb/details/config-vc.h>
#else
#  include <odb/details/config.h>
#endif

// no post

#endif // ODB_DETAILS_CONFIG_HXX
