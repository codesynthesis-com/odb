# file      : Makefile.am
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

SUBDIRS = libcommon tracer common

if DATABASE_MYSQL
SUBDIRS += mysql
endif

EXTRA_DIST = __file__(extra_dist)
ACLOCAL_AMFLAGS = -I m4
