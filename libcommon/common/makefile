# file      : libcommon/common/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

# Options file.
#
cli_tun := options.cli
cxx_tun := common.cxx

cxx_obj   := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o) $(cli_tun:.cli=.o))
cxx_od    := $(cxx_obj:.o=.o.d)

common.l             := $(out_base)/common.l
common.l.cpp-options := $(out_base)/common.l.cpp-options

default := $(out_base)/
dist    := $(out_base)/.dist
clean   := $(out_base)/.clean

# Import.
#
$(call import,\
  $(scf_root)/import/cli/stub.make,\
  cli: cli,cli-rules: cli_rules)

$(call import,\
  $(scf_root)/import/libodb/stub.make,\
  l: odb.l,cpp-options: odb.l.cpp-options)

ifdef db_id
$(call import,\
  $(scf_root)/import/libodb-$(db_id)/stub.make,\
  l: odb_db.l,cpp-options: odb_db.l.cpp-options)
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

genf := $(cli_tun:.cli=.hxx) $(cli_tun:.cli=.ixx) $(cli_tun:.cli=.cxx)
gen  := $(addprefix $(out_base)/,$(genf))

$(gen): $(cli)
$(gen): cli := $(cli)
$(gen): cli_options +=   \
--generate-specifier     \
--generate-file-scanner  \
--guard-prefix LIBCOMMON_COMMON

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

$(out_base)/config.h: $(dcf_root)/configuration-dynamic.make
	@echo '// file      : libcommon/config.h'                        >$@
	@echo '// author    : automatically generated'                  >>$@
	@echo ''                                                        >>$@
	@echo '#ifndef LIBCOMMON_COMMON_CONFIG_H'                       >>$@
	@echo '#define LIBCOMMON_COMMON_CONFIG_H'                       >>$@
	@echo ''                                                        >>$@
ifeq ($(db_id),mysql)
	@echo '#define DB_ID_MYSQL 1'                                   >>$@
endif
	@echo ''                                                        >>$@
	@echo '#endif // LIBCOMMON_COMMON_CONFIG_H'                     >>$@

# Convenience alias for default target.
#
$(out_base)/: $(common.l)

# Dist.
#
$(dist): export sources := $(cxx_tun) $(cli_tun:.cli=.cxx)
$(dist): export headers = $(subst $(src_base)/,,$(shell find $(src_base) \
-name '*.hxx' -o -name '*.ixx' -o -name '*.txx'))
$(dist): data_dist := $(cli_tun)
$(dist): export extra_dist := $(data_dist) $(call vc9projs,libcommon) \
$(call vc10projs,libcommon)

$(dist): $(gen)
	$(call dist-data,$(sources) $(headers) $(data_dist) config.h.in)
	$(call meta-automake)
	$(call meta-vc9projs,libcommon)
	$(call meta-vc10projs,libcommon)

# Clean.
#
$(clean): $(common.l).o.clean        \
  $(common.l.cpp-options).clean      \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))  \
  $(addprefix $(out_base)/,$(cli_tun:.cli=.cxx.cli.clean))
	$(call message,rm $$1,rm -f $$1,$(out_base)/config.h)

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(common.l): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := config.h $(genf)
$(clean): $(out_base)/.gitignore.clean

$(call include,$(bld_root)/git/gitignore.make)
endif

# How to.
#
$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vc9proj.make)
$(call include,$(bld_root)/meta/vc10proj.make)
$(call include,$(bld_root)/meta/automake.make)

$(call include,$(cli_rules))
$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-l.make)
