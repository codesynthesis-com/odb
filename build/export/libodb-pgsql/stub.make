# file      : build/export/libodb-pgsql/stub.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/pgsql/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/pgsql/odb-pgsql.l,\
  cpp-options: $(out_root)/odb/pgsql/odb-pgsql.l.cpp-options)
