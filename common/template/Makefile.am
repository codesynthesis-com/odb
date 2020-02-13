# file      : common/template/Makefile.am
# license   : GNU GPL v2; see accompanying LICENSE file

EXTRA_DIST = __file__(extra_dist)

noinst_PROGRAMS = driver
driver_SOURCES = driver.cxx __path__(extra_sources) __path__(extra_headers)
LDADD = $(top_builddir)/libcommon/common/libcommon.la
AM_CPPFLAGS = -I'$(top_builddir)/libcommon' -I'$(top_srcdir)/libcommon'
AM_CPPFLAGS += -I'$(builddir)' -I'$(srcdir)'

TESTS=$(top_builddir)/tester
TESTS_ENVIRONMENT=top_builddir=$(top_builddir); export top_builddir;

__ifelse__(__value__(odb_options),,,
# ODB compilation.
#
ODB = @ODB@
ODBFLAGS = @ODBFLAGS@
ODBCPPFLAGS = @ODBCPPFLAGS@

if HAVE_CXX11
ODBFLAGS += --std c++11
endif

nodist_driver_SOURCES =
BUILT_SOURCES =
CLEANFILES =

__foreach_w__(__f,__path__(odb_header_stem),
driver_SOURCES += __f.hxx
nodist_driver_SOURCES += __f-odb.cxx
BUILT_SOURCES += __f-odb.hxx
CLEANFILES += __f-odb.hxx __f-odb.ixx __f-odb.cxx

__f-odb.hxx: __f.hxx
	$(ODB) $(AM_CPPFLAGS) $(ODBCPPFLAGS) $(CPPFLAGS) $(ODBFLAGS) --database @database@ __value__(odb_options) $<

))
