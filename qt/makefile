# file      : qt/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

all_dirs := common mysql sqlite pgsql oracle mssql
dirs := common
dirs += $(db_id)

default := $(out_base)/
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(dirs)))

$(dist): export extra_dist := build.bat
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_dirs)))
	$(call meta-vctest,common/qt-common-qt4-mysql-vc10.sln,build.bat)
	$(call meta-automake)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(dirs)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_dirs)))

$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach d,$(all_dirs),$(call import,$(src_base)/$d/makefile))
else
$(foreach d,$(dirs),$(call import,$(src_base)/$d/makefile))
endif
