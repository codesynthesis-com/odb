# file      : build/export/libodb-qt/stub.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(src_root)/odb/qt/makefile,$(out_root))

$(call export,\
  l: $(out_root)/odb/qt/odb-qt.l,\
  cpp-options: $(out_root)/odb/qt/odb-qt.l.cpp-options)
