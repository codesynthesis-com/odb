# file      : Makefile.am
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

SUBDIRS = libcommon tracer common

if DB_ID_MYSQL
SUBDIRS += mysql
endif

EXTRA_DIST = __file__(extra_dist)
ACLOCAL_AMFLAGS = -I m4
