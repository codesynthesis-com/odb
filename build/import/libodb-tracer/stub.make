# file      : build/import/libodb-tracer/stub.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-tracer/configuration-rules.make,$(dcf_root))

libodb_tracer_installed :=

$(call -include,$(dcf_root)/import/libodb-tracer/configuration-dynamic.make)

ifdef libodb_tracer_installed

ifeq ($(libodb_tracer_installed),y)

$(call export,l: -lodb-tracer -lodb,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-tracer/stub.make)

endif

else

.NOTPARALLEL:

endif
