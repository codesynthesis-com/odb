# file      : libcommon/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

dirs := common

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(dirs)))

$(dist): export dirs := $(dirs)
$(dist): export extra_dist := $(call vc8slns,libcommon) \
$(call vc9slns,libcommon) $(call vc10slns,libcommon) $(call vc11slns,libcommon)
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(dirs)))
	$(call meta-automake)
	$(call meta-vc8slns,libcommon)
	$(call meta-vc9slns,libcommon)
	$(call meta-vc10slns,libcommon)
	$(call meta-vc11slns,libcommon)
	$(call meta-vc12slns,libcommon)

$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(dirs)))

$(call include,$(bld_root)/meta/vc8sln.make)
$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vc11sln.make)
$(call include,$(bld_root)/meta/vc12sln.make)
$(call include,$(bld_root)/meta/automake.make)

$(foreach d,$(dirs),$(call import,$(src_base)/$d/makefile))
