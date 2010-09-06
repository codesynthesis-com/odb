dnl file      : m4/libtool-link.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl
dnl CXX_LIBTOOL_LINK_IFELSE (input, [action-if-true], [action-if-false])
dnl
dnl Similar to AC_LINK_IFELSE except it uses libtool to perform the
dnl linking and it does this using the C++ compiler.
dnl
AC_DEFUN([CXX_LIBTOOL_LINK_IFELSE],[
AC_LANG_SAVE
save_CXX="$CXX"
CXX="./libtool --tag=CXX --mode=link $CXX -no-install"
AC_LANG(C++)

if test -d .libs; then
  delete_libs_dir=no
else
  delete_libs_dir=yes
fi

AC_LINK_IFELSE([$1], [$2], [$3])

if test x"$delete_libs_dir" != xyes; then
  rm -rf .libs
fi

CXX="$save_CXX"
AC_LANG_RESTORE])dnl
