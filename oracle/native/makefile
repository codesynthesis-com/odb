# file      : oracle/native/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx_tun := driver.cxx
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o))
cxx_od  := $(cxx_obj:.o=.o.d)

common.l             := $(out_root)/libcommon/common/common.l
common.l.cpp-options := $(out_root)/libcommon/common/common.l.cpp-options

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
$(dist): export extra_dist := $(data_dist) \
$(name)-vc8.vcproj $(name)-vc9.vcproj \
$(name)-vc10.vcxproj $(name)-vc10.vcxproj.filters \
$(name)-vc11.vcxproj $(name)-vc11.vcxproj.filters
$(dist):
	$(call dist-data,$(sources) $(data_dist))
	$(call meta-automake,../template/Makefile.am)
	$(call meta-vc8proj,../template/template-vc8.vcproj,$(name)-vc8.vcproj)
	$(call meta-vc9proj,../template/template-vc9.vcproj,$(name)-vc9.vcproj)
	$(call meta-vc10proj,../template/template-vc10.vcxproj,$(name)-vc10.vcxproj)
	$(call meta-vc11proj,../template/template-vc11.vcxproj,$(name)-vc11.vcxproj)

# Test.
#
$(eval $(call test-schemaless-rule))

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
$(call include,$(bld_root)/meta/vc8proj.make)
$(call include,$(bld_root)/meta/vc9proj.make)
$(call include,$(bld_root)/meta/vc10proj.make)
$(call include,$(bld_root)/meta/vc11proj.make)
$(call include,$(bld_root)/meta/automake.make)

$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-e.make)

# Dependencies.
#
$(call import,$(src_root)/libcommon/makefile)
