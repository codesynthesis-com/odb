# file      : odb/pgsql/Makefile.am
# license   : GNU GPL v2; see accompanying LICENSE file

lib_LTLIBRARIES = libodb-pgsql.la
libodb_pgsql_la_SOURCES = __path__(sources)

odbpgsqlincludedir = $(includedir)/odb/pgsql/
nobase_odbpgsqlinclude_HEADERS = __path__(headers)
nobase_nodist_odbpgsqlinclude_HEADERS = details/config.h

EXTRA_DIST = __file__(extra_dist)

AM_CPPFLAGS= -I'$(top_builddir)' -I'$(top_srcdir)' -DLIBODB_PGSQL_DYNAMIC_LIB
AM_LDFLAGS = -release __value__(interface_version) -no-undefined
