# file      : odb/boost/Makefile.am
# license   : GNU GPL v2; see accompanying LICENSE file

lib_LTLIBRARIES = libodb-boost.la
libodb_boost_la_SOURCES = __path__(sources)

odbboostincludedir = $(includedir)/odb/boost/
nobase_odbboostinclude_HEADERS = __path__(headers) __path__(options)
nobase_nodist_odbboostinclude_HEADERS = details/config.h

EXTRA_DIST = __file__(extra_dist)

AM_CPPFLAGS= -I'$(top_builddir)' -I'$(top_srcdir)' -DLIBODB_BOOST_DYNAMIC_LIB
AM_LDFLAGS = -release __value__(interface_version) -no-undefined
