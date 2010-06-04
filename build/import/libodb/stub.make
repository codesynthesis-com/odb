# file      : build/import/libodb/stub.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb/configuration-rules.make,$(dcf_root))

libodb_installed :=

$(call -include,$(dcf_root)/import/libodb/configuration-dynamic.make)

ifdef libodb_installed

ifeq ($(libodb_installed),y)

$(call export,l: -lodb,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb/stub.make)

endif

else

.NOTPARALLEL:

endif
