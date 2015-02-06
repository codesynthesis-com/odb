# file      : odb/mssql/Makefile.am
# copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

lib_LTLIBRARIES = libodb-mssql.la
libodb_mssql_la_SOURCES = __path__(sources)

odbmssqlincludedir = $(includedir)/odb/mssql/
nobase_odbmssqlinclude_HEADERS = __path__(headers)
nobase_nodist_odbmssqlinclude_HEADERS = details/config.h

EXTRA_DIST = __file__(extra_dist)

AM_CPPFLAGS= -I'$(top_builddir)' -I'$(top_srcdir)' -DLIBODB_MSSQL_DYNAMIC_LIB
AM_LDFLAGS = -release __value__(interface_version) -no-undefined
