# file      : oracle/database/makefile
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx_tun := driver.cxx
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o))
cxx_od  := $(cxx_obj:.o=.o.d)

common.l             := $(out_root)/libcommon/common/common.l
common.l.cpp-options := $(out_root)/libcommon/common/common.l.cpp-options

driver  := $(out_base)/driver
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

# Build.
#
$(driver): $(cxx_obj) $(common.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base) -I$(src_base)
$(cxx_obj) $(cxx_od): $(common.l.cpp-options)

$(call include-dep,$(cxx_od))

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
$(dist): sources := $(cxx_tun)
$(dist): data_dist := test.std
$(dist): export name := $(subst /,-,$(subst $(src_root)/oracle/,,$(src_base)))
$(dist): export extra_dist := $(data_dist) $(name)-vc9.vcproj \
$(name)-vc10.vcxproj $(name)-vc10.vcxproj.filters
$(dist):
	$(call dist-data,$(sources) $(data_dist))
	$(call meta-automake,../template/Makefile.am)
	$(call meta-vc9proj,../template/template-vc9.vcproj,$(name)-vc9.vcproj)
	$(call meta-vc10proj,../template/template-vc10.vcxproj,$(name)-vc10.vcxproj)

# Test.
#
$(test): $(driver) $(src_base)/test.std
	$(call message,test $<,$< --options-file $(dcf_root)/db.options \
>$(out_base)/test.out)
	$(call message,,diff -u $(src_base)/test.std $(out_base)/test.out)
	$(call message,,rm -f $(out_base)/test.out)

# Clean.
#
$(clean):                            \
  $(driver).o.clean                  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))
	$(call message,,rm -f $(out_base)/test.out)

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(driver): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := driver
$(clean): $(out_base)/.gitignore.clean

$(call include,$(bld_root)/git/gitignore.make)
endif

# How to.
#
$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vc9proj.make)
$(call include,$(bld_root)/meta/vc10proj.make)
$(call include,$(bld_root)/meta/automake.make)

$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-e.make)

# Dependencies.
#
$(call import,$(src_root)/libcommon/makefile)
