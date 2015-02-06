# file      : build/import/libodb-qt/stub.make
# copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-qt/configuration-rules.make,$(dcf_root))

libodb_qt_installed :=

$(call -include,$(dcf_root)/import/libodb-qt/configuration-dynamic.make)

ifdef libodb_qt_installed

ifeq ($(libodb_qt_installed),y)

$(call export,l: -lodb-qt -lodb,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-qt/stub.make)

endif

else

.NOTPARALLEL:

endif
