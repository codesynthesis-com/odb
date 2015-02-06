dnl file      : m4/diff.m4
dnl copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl DIFF_TOOL
dnl
AC_DEFUN([DIFF_TOOL], [
diff_found=no

AC_ARG_VAR([DIFF],[diff command])
AC_ARG_VAR([DIFFFLAGS],[diff flags])

AC_ARG_WITH(
  [diff],
  [AC_HELP_STRING([--with-diff=PATH],[path to the diff program])],
  [diff_path=${withval}],
  [diff_path=])

AC_MSG_CHECKING([for diff])

if test x"$DIFF" = x; then
 if test x"$diff_path" != x; then
   AS_SET_CATFILE([abs_diff_path], [$ac_pwd], [$diff_path])
   DIFF="$abs_diff_path"
 else
   DIFF=diff
 fi
fi

cat >conftest.txt

$DIFF conftest.txt conftest.txt 2>/dev/null 1>&2

if test x"$?" = x0; then
  AC_MSG_RESULT([$DIFF])
else
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([diff command is not found; consider setting the DIFF variable or using --with-diff=PATH])
fi

AC_MSG_CHECKING([whether $DIFF accepts -u])

$DIFF -u conftest.txt conftest.txt 2>/dev/null 1>&2

if test x"$?" = x0; then
  AC_MSG_RESULT([yes])
  DIFFFLAGS="$DIFFFLAGS -u"
else
  AC_MSG_RESULT([no])
fi

rm -f conftest.txt

])dnl
