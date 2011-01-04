# file      : makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))build/bootstrap.make

all_dirs := libcommon common tracer mysql
dirs := common tracer

ifeq ($(db_id),mysql)
dirs += mysql
endif

default := $(out_base)/
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(dirs)))

$(dist): data_dist := GPLv2 LICENSE README INSTALL version test.bat \
tester.bat mysql-driver.bat mysql.options
$(dist): exec_dist := bootstrap tester.in
$(dist): export extra_dist := $(data_dist) $(exec_dist) build.bat
$(dist): export version = $(shell cat $(src_root)/version)

$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_dirs)))
	$(call dist-data,$(data_dist))
	$(call dist-exec,$(exec_dist))
	$(call dist-dir,m4)
	$(call meta-vctest,tracer/tracer-vc10.sln,build.bat)
	$(call meta-automake)
	$(call meta-autoconf)	

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(dirs)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_dirs)))

$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)
$(call include,$(bld_root)/meta/autoconf.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach d,$(all_dirs),$(call import,$(src_base)/$d/makefile))
else
$(foreach d,$(dirs),$(call import,$(src_base)/$d/makefile))
endif
