# file      : build/import/libodb-oracle/stub.make
# copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-oracle/configuration-rules.make,$(dcf_root))

libodb_oracle_installed :=

$(call -include,$(dcf_root)/import/libodb-oracle/configuration-dynamic.make)

ifdef libodb_oracle_installed

ifeq ($(libodb_oracle_installed),y)

$(call export,l: -lodb-oracle -lodb -lclntsh,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-oracle/stub.make)

endif

else

.NOTPARALLEL:

endif
