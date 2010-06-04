# file      : build/import/odb/stub.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/odb/configuration-rules.make,$(dcf_root))

odb_installed :=

$(call -include,$(dcf_root)/import/odb/configuration-dynamic.make)

ifdef odb_installed

ifeq ($(odb_installed),y)

$(call export,odb: odb,odb-rules: $(scf_root)/import/odb/hxx-cxx.make)

else

# Include export stub.
#
$(call include,$(scf_root)/export/odb/stub.make)

endif

else

.NOTPARALLEL:

endif
