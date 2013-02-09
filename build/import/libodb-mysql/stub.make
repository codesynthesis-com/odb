# file      : build/import/libodb-mysql/stub.make
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodb-mysql/configuration-rules.make,$(dcf_root))

libodb_mysql_installed :=

$(call -include,$(dcf_root)/import/libodb-mysql/configuration-dynamic.make)

ifdef libodb_mysql_installed

ifeq ($(libodb_mysql_installed),y)

$(call export,l: -lodb-mysql -lodb -lmysqlclient_r,cpp-options: )

else

# Include export stub.
#
$(call include,$(scf_root)/export/libodb-mysql/stub.make)

endif

else

.NOTPARALLEL:

endif
