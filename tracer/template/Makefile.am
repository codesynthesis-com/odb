# file      : tracer/template/Makefile.am
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

noinst_PROGRAMS = driver
driver_SOURCES = driver.cxx __path__(extra_sources) __path__(extra_headers)

TESTS=$(top_builddir)/tester
TESTS_ENVIRONMENT=top_builddir=$(top_builddir);

m4_ifelse(__value__(odb_options),,,

# ODB compilation.
#
driver_SOURCES += test.hxx test-odb.cxx
BUILT_SOURCES = test-odb.hxx
CLEANFILES = test-odb.hxx test-odb.ixx test-odb.cxx

ODB = @ODB@
ODBFLAGS = @ODBFLAGS@

test-odb.hxx: test.hxx
	$(ODB) $(ODBCPPFLAGS) $(CPPFLAGS) $(ODBFLAGS) __value__(odb_options) $<
)
