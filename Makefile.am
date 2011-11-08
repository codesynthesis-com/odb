# file      : Makefile.am
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

SUBDIRS = libcommon common

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

EXTRA_DIST = __file__(extra_dist)
ACLOCAL_AMFLAGS = -I m4
