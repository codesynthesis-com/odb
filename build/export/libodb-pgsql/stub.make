# file      : build/export/libodb-pgsql/stub.make
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/pgsql/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/pgsql/odb-pgsql.l,\
  cpp-options: $(out_root)/odb/pgsql/odb-pgsql.l.cpp-options)
