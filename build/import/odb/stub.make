# file      : build/import/odb/stub.make
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
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
