# file      : common/Makefile.am
# license   : GNU GPL v2; see accompanying LICENSE file

SUBDIRS = __path__(dirs)

if ODB_TESTS_THREADS
SUBDIRS += __path__(thread_dirs)
endif

if HAVE_CXX11
SUBDIRS += __path__(cxx11_dirs)
endif

EXTRA_DIST = __file__(extra_dist)
