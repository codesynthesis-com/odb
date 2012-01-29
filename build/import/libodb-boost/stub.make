# file      : build/import/libodb-boost/stub.make
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-boost/configuration-rules.make,$(dcf_root))

libodb_boost_installed :=

$(call -include,$(dcf_root)/import/libodb-boost/configuration-dynamic.make)

ifdef libodb_boost_installed

ifeq ($(libodb_boost_installed),y)

$(call export,l: -lodb-boost -lodb,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-boost/stub.make)

endif

else

.NOTPARALLEL:

endif
