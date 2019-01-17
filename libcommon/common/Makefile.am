# file      : libcommon/common/Makefile.am
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = __path__(sources) __path__(headers)

EXTRA_DIST = __file__(extra_dist)

AM_CPPFLAGS = -I'$(top_builddir)/libcommon' -I'$(top_srcdir)/libcommon'
AM_CPPFLAGS += -DLIBCOMMON_DYNAMIC_LIB
AM_LDFLAGS = -no-undefined -rpath '$(libdir)'
