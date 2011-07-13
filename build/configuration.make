# file      : build/configuration.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/configuration-rules.make,$(dcf_root))

# Dynamic configuration.
#
db_id :=

$(call -include,$(dcf_root)/configuration-dynamic.make)

ifdef db_id

ifeq ($(db_id),mysql)
db_macro := DATABASE_MYSQL
endif

ifeq ($(db_id),sqlite)
db_macro := DATABASE_SQLITE
endif

ifeq ($(db_id),pgsql)
db_macro := DATABASE_PGSQL
endif

$(out_root)/%: db_id    := $(db_id)
$(out_root)/%: db_macro := $(db_macro)

else

.NOTPARALLEL:

endif
