# file      : libcommon/common/makefile
# copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx_tun := common.cxx

cxx_obj   := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o))
cxx_od    := $(cxx_obj:.o=.o.d)

common.l             := $(out_base)/common.l
common.l.cpp-options := $(out_base)/common.l.cpp-options

# Import.
#
$(call import,\
  $(scf_root)/import/libodb/stub.make,\
  l: odb.l,cpp-options: odb.l.cpp-options)

ifdef db_id
ifneq ($(db_id),common)
$(call import,\
  $(scf_root)/import/libodb-$(db_id)/stub.make,\
  l: odb_db.l,cpp-options: odb_db.l.cpp-options)
else
# Import all database runtimes.
#
$(call import,\
  $(scf_root)/import/libodb-mysql/stub.make,\
  l: odb_mysql.l,cpp-options: odb_mysql.l.cpp-options)

$(call import,\
  $(scf_root)/import/libodb-sqlite/stub.make,\
  l: odb_sqlite.l,cpp-options: odb_sqlite.l.cpp-options)

$(call import,\
  $(scf_root)/import/libodb-pgsql/stub.make,\
  l: odb_pgsql.l,cpp-options: odb_pgsql.l.cpp-options)

$(call import,\
  $(scf_root)/import/libodb-oracle/stub.make,\
  l: odb_oracle.l,cpp-options: odb_oracle.l.cpp-options)

$(call import,\
  $(scf_root)/import/libodb-mssql/stub.make,\
  l: odb_mssql.l,cpp-options: odb_mssql.l.cpp-options)

odb_db.l :=     \
$(odb_mysql.l)  \
$(odb_sqlite.l) \
$(odb_pgsql.l)  \
$(odb_oracle.l) \
$(odb_mssql.l)

odb_db.l.cpp-options :=     \
$(odb_mysql.l.cpp-options)  \
$(odb_sqlite.l.cpp-options) \
$(odb_pgsql.l.cpp-options)  \
$(odb_oracle.l.cpp-options) \
$(odb_mssql.l.cpp-options)
endif
endif

ifeq ($(odb_db.l.cpp-options),)
odb_db.l.cpp-options := $(out_base)/.unbuildable
endif

# Build.
#
$(common.l): $(cxx_obj) $(odb.l) $(odb_db.l)

$(cxx_obj) $(cxx_od): $(common.l.cpp-options) $(out_base)/config.h
$(common.l.cpp-options): value := -I$(out_root)/libcommon -I$(src_root)/libcommon
$(common.l.cpp-options): $(odb_db.l.cpp-options) $(odb.l.cpp-options)

$(call include,$(bld_root)/cxx/standard.make) # cxx_standard

ifdef db_id
ifdef cxx_standard
$(out_base)/config.h: | $(out_base)/.
	@echo '/* file      : libcommon/common/config.h'                 >$@
	@echo ' * note      : automatically generated'                  >>$@
	@echo ' */'                                                     >>$@
	@echo ''                                                        >>$@
	@echo '#ifndef LIBCOMMON_COMMON_CONFIG_H'                       >>$@
	@echo '#define LIBCOMMON_COMMON_CONFIG_H'                       >>$@
	@echo ''                                                        >>$@
ifeq ($(db_id),mysql)
	@echo '#define DATABASE_MYSQL 1'                                >>$@
else ifeq ($(db_id),sqlite)
	@echo '#define DATABASE_SQLITE 1'                               >>$@
else ifeq ($(db_id),pgsql)
	@echo '#define DATABASE_PGSQL 1'                                >>$@
else ifeq ($(db_id),oracle)
	@echo '#define DATABASE_ORACLE 1'                               >>$@
else ifeq ($(db_id),mssql)
	@echo '#define DATABASE_MSSQL 1'                                >>$@
else ifeq ($(db_id),common)
	@echo '#define DATABASE_COMMON 1'                               >>$@
endif
ifeq ($(cxx_standard),c++11)
	@echo '#define HAVE_CXX11 1'                                    >>$@
endif
	@echo '#define HAVE_TR1_MEMORY 1'                               >>$@
	@echo ''                                                        >>$@
	@echo '#endif /* LIBCOMMON_COMMON_CONFIG_H */'                  >>$@
endif
endif

$(call include-dep,$(cxx_od),$(cxx_obj),$(out_base)/config.h)

# Convenience alias for default target.
#
$(out_base)/: $(common.l)

# Dist.
#
$(dist): export sources := $(cxx_tun)
$(dist): export headers = $(subst $(src_base)/,,$(shell find $(src_base) \
-name '*.hxx' -o -name '*.ixx' -o -name '*.txx'))
$(dist): data_dist := config.h.in config-vc.h
$(dist): export extra_dist := $(data_dist) $(call vc8projs,libcommon) \
$(call vc9projs,libcommon) $(call vc10projs,libcommon) \
$(call vc11projs,libcommon) $(call vc12projs,libcommon)

$(dist):
	$(call dist-data,$(sources) $(headers) $(data_dist))
	$(call meta-automake)
	$(call meta-vc8projs,libcommon)
	$(call meta-vc9projs,libcommon)
	$(call meta-vc10projs,libcommon)
	$(call meta-vc11projs,libcommon)
	$(call meta-vc12projs,libcommon)

# Clean.
#
$(clean): $(common.l).o.clean        \
  $(common.l.cpp-options).clean      \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))
	$(call message,rm $$1,rm -f $$1,$(out_base)/config.h)

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(common.l): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := config.h
$(clean): $(out_base)/.gitignore.clean

$(call include,$(bld_root)/git/gitignore.make)
endif

# How to.
#
$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vc8proj.make)
$(call include,$(bld_root)/meta/vc9proj.make)
$(call include,$(bld_root)/meta/vc10proj.make)
$(call include,$(bld_root)/meta/vc11proj.make)
$(call include,$(bld_root)/meta/vc12proj.make)
$(call include,$(bld_root)/meta/automake.make)

$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-l.make)
