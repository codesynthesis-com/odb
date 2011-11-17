# file      : build/export/libodb-mssql/stub.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/mssql/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/mssql/odb-mssql.l,\
  cpp-options: $(out_root)/odb/mssql/odb-mssql.l.cpp-options)
