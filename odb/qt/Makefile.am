# file      : odb/qt/Makefile.am
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

lib_LTLIBRARIES = libodb-qt.la
libodb_qt_la_SOURCES = __path__(sources)

odbqtincludedir = $(includedir)/odb/qt/
nobase_odbqtinclude_HEADERS = __path__(headers) __path__(options)
nobase_nodist_odbqtinclude_HEADERS = details/config.h

EXTRA_DIST = __file__(extra_dist)

AM_CPPFLAGS= -I'$(top_builddir)' -I'$(top_srcdir)' -DLIBODB_QT_DYNAMIC_LIB
AM_LDFLAGS = -release __value__(interface_version) -no-undefined
