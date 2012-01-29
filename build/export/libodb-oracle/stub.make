# file      : build/export/libodb-oracle/stub.make
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/oracle/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/oracle/odb-oracle.l,\
  cpp-options: $(out_root)/odb/oracle/odb-oracle.l.cpp-options)
