# file      : build/import/libodbc/stub.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Boris Kolpackov
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libodbc/configuration-rules.make,$(dcf_root))

libodbc_manager :=

$(call -include,$(dcf_root)/import/libodbc/configuration-dynamic.make)

ifdef libodbc_manager

ifeq ($(libodbc_manager),windows)
$(call export,l: -lodbc32,cpp-options: )
else
$(call export,l: -lodbc,cpp-options: )
endif

else

.NOTPARALLEL:

endif
