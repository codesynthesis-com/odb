# file      : build/export/libodb-boost/stub.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/boost/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/boost/odb-boost.l,\
  cpp-options: $(out_root)/odb/boost/odb-boost.l.cpp-options)
