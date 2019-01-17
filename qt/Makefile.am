# file      : qt/Makefile.am
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

SUBDIRS = common

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
