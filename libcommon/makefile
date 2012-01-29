# file      : libcommon/makefile
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

dirs := common

default   := $(out_base)/
dist      := $(out_base)/.dist
clean     := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(dirs)))

$(dist): export dirs := $(dirs)
$(dist): export extra_dist := $(call vc9slns,libcommon) \
$(call vc10slns,libcommon)
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(dirs)))
	$(call meta-automake)
	$(call meta-vc9slns,libcommon)
	$(call meta-vc10slns,libcommon)

$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(dirs)))

$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/automake.make)

$(foreach d,$(dirs),$(call import,$(src_base)/$d/makefile))
