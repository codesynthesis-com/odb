# file      : build/export/libodb-mssql/stub.make
# license   : ODB NCUEL; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/mssql/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/mssql/odb-mssql.l,\
  cpp-options: $(out_root)/odb/mssql/odb-mssql.l.cpp-options)
