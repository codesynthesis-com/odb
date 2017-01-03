# file      : odb/oracle/Makefile.am
# copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

lib_LTLIBRARIES = libodb-oracle.la
libodb_oracle_la_SOURCES = __path__(sources)

odboracleincludedir = $(includedir)/odb/oracle/
nobase_odboracleinclude_HEADERS = __path__(headers)
nobase_nodist_odboracleinclude_HEADERS = details/config.h

EXTRA_DIST = __file__(extra_dist)

AM_CPPFLAGS= -I'$(top_builddir)' -I'$(top_srcdir)' -DLIBODB_ORACLE_DYNAMIC_LIB
AM_LDFLAGS = -release __value__(interface_version) -no-undefined
