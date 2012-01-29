# file      : build/import/libodb-sqlite/stub.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-sqlite/configuration-rules.make,$(dcf_root))

libodb_sqlite_installed :=

$(call -include,$(dcf_root)/import/libodb-sqlite/configuration-dynamic.make)

ifdef libodb_sqlite_installed

ifeq ($(libodb_sqlite_installed),y)

$(call export,l: -lodb-sqlite -lodb -lsqlite3,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-sqlite/stub.make)

endif

else

.NOTPARALLEL:

endif
