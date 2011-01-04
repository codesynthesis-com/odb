dnl file      : m4/threads.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
AC_DEFUN([THREADS],[

threads_thread_keyword=no

AC_ARG_ENABLE(
  [threads],
  AS_HELP_STRING([--disable-threads], [disable threads (enabled by default)]),
  [AS_IF([test x"$enableval" = xno], [threads=none], [threads=check])],
  [threads=check])

# If thread support is not disabled by the user, figure out what we can use.
#
if test x$threads = xcheck; then
  case $host_os in
    windows* | mingw*)
      case $host_os in
        mingw*)
          CXXFLAGS="$CXXFLAGS -mthreads"
          ;;
      esac
      threads=win32
      ;;
    *)
      ACX_PTHREAD

      if test x$acx_pthread_ok = xyes; then
        threads=posix
        LIBS="$LIBS $PTHREAD_LIBS"
        CXXFLAGS="$CXXFLAGS $PTHREAD_CXXFLAGS"

        # Check if we can use the __thread keyword.
        #
        AC_MSG_CHECKING([for __thread keyword])

        CXX_LIBTOOL_LINK_IFELSE(
          AC_LANG_SOURCE([[
            __thread int tls_var;

            int
            main ()
            {
              tls_var = 0;
            }
          ]]),
          [threads_thread_keyword=yes])

        AC_MSG_RESULT([$threads_thread_keyword])
      fi
      ;;
  esac
fi

if test x$threads = xcheck; then
  AC_MSG_ERROR([thread support not available; use --disable-threads to force single-threaded mode])
fi
])dnl
