# file      : build/import/libodb-pgsql/stub.make
# copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-pgsql/configuration-rules.make,$(dcf_root))

libodb_pgsql_installed :=

$(call -include,$(dcf_root)/import/libodb-pgsql/configuration-dynamic.make)

ifdef libodb_pgsql_installed

ifeq ($(libodb_pgsql_installed),y)

$(call export,l: -lodb-pgsql -lodb -lpq,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-pgsql/stub.make)

endif

else

.NOTPARALLEL:

endif
