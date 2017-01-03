# file      : common/section/polymorphism/makefile
# copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../../build/bootstrap.make

cxx_tun := driver.cxx
odb_hdr := test.hxx
genf    := $(call odb-gen,$(odb_hdr))
gen     := $(addprefix $(out_base)/,$(genf))
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o)) $(filter %.o,$(gen:.cxx=.o))
cxx_od  := $(cxx_obj:.o=.o.d)

common.l             := $(out_root)/libcommon/common/common.l
common.l.cpp-options := $(out_root)/libcommon/common/common.l.cpp-options

# Import.
#
$(call import,\
  $(scf_root)/import/odb/stub.make,\
  odb: odb,odb-rules: odb_rules)

# Build.
#
$(driver): $(cxx_obj) $(common.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base) -I$(src_base)
$(cxx_obj) $(cxx_od): $(common.l.cpp-options)

$(gen): $(odb)
$(gen): odb := $(odb)
$(gen) $(dist): export odb_options += --generate-schema --generate-query \
--table-prefix t_section_p_
$(gen): cpp_options := -I$(src_base)
$(gen): $(common.l.cpp-options)

ifneq ($(db_id),common)
$(gen): odb_options += --database $(db_id)
else
$(gen): odb_options += --multi-database dynamic
endif

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
name := $(subst /,-,$(subst $(src_root)/common/,,$(src_base)))

$(dist): sources := $(cxx_tun)
$(dist): headers := $(odb_hdr)
$(dist): data_dist := test.std
$(dist): export name := $(name)
$(dist): export extra_dist := $(data_dist) $(call vc8projs,$(name)) \
$(call vc9projs,$(name)) $(call vc10projs,$(name)) $(call vc11projs,$(name)) \
$(call vc12projs,$(name))
$(dist):
	$(call dist-data,$(sources) $(headers) $(data_dist))
	$(call meta-automake,../../template/Makefile.am)
	$(call meta-vc8projs,../../template/template,$(name))
	$(call meta-vc9projs,../../template/template,$(name))
	$(call meta-vc10projs,../../template/template,$(name))
	$(call meta-vc11projs,../../template/template,$(name))
	$(call meta-vc12projs,../../template/template,$(name))

# Test.
#
ifneq ($(db_id),common)
$(eval $(call test-rule))
else
$(foreach d,$(databases),$(eval $(call test-rule,$d)))
endif

# Clean.
#
$(clean):                            \
  $(driver).o.clean                  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))  \
  $(addsuffix .hxx.clean,$(filter %.cxx,$(gen)))
	$(call message,,rm -f $(out_base)/test.out)

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(driver): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := driver $(genf)
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

$(call include,$(bld_root)/cxx/standard.make) # cxx_standard
ifdef cxx_standard
$(gen): odb_options += --std $(cxx_standard)
$(call include,$(odb_rules))
endif

$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-e.make)

# Dependencies.
#
$(call import,$(src_root)/libcommon/makefile)
