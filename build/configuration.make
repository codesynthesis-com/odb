# file      : build/configuration.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/configuration-rules.make,$(dcf_root))

# Dynamic configuration.
#
db_id :=

$(call -include,$(dcf_root)/configuration-dynamic.make)

ifdef db_id

ifeq ($(db_id),mysql)
db_macro := DATABASE_MYSQL
else ifeq ($(db_id),sqlite)
db_macro := DATABASE_SQLITE
else ifeq ($(db_id),pgsql)
db_macro := DATABASE_PGSQL
else ifeq ($(db_id),oracle)
db_macro := DATABASE_ORACLE
else ifeq ($(db_id),mssql)
db_macro := DATABASE_MSSQL
endif

$(out_root)/%: db_id    := $(db_id)
$(out_root)/%: db_macro := $(db_macro)

else

.NOTPARALLEL:

endif
