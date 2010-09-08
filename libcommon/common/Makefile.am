# file      : libcommon/common/Makefile.am
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = __path__(sources) __path__(headers)

EXTRA_DIST = __file__(extra_dist)

libcommon_la_CPPFLAGS= -I'$(top_builddir)/libcommon' -I'$(top_srcdir)/libcommon' -DLIBCOMMON_DYNAMIC_LIB
libcommon_la_LDFLAGS = -no-undefined -rpath '$(libdir)'
