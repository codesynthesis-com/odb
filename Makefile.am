# file      : Makefile.am
# license   : GNU GPL v2; see accompanying LICENSE file

SUBDIRS = libcommon common evolution

if ODB_TESTS_BOOST
SUBDIRS += boost
endif

if ODB_TESTS_QT
SUBDIRS += qt
endif

if DATABASE_MYSQL
SUBDIRS += mysql
endif

if DATABASE_SQLITE
SUBDIRS += sqlite
endif

if DATABASE_PGSQL
SUBDIRS += pgsql
endif

if DATABASE_ORACLE
SUBDIRS += oracle
endif

if DATABASE_MSSQL
SUBDIRS += mssql
endif

EXTRA_DIST = __file__(extra_dist)
ACLOCAL_AMFLAGS = -I m4
