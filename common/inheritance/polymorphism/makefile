# file      : common/inheritance/polymorphism/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../../build/bootstrap.make

cxx_tun := driver.cxx
odb_hdr := test1.hxx test2.hxx test3.hxx test4.hxx test5.hxx test6.hxx \
test7.hxx test8.hxx test9.hxx test10.hxx test11.hxx test12.hxx test13.hxx \
test14.hxx
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o) $(odb_hdr:.hxx=-odb.o))
cxx_od  := $(cxx_obj:.o=.o.d)

common.l             := $(out_root)/libcommon/common/common.l
common.l.cpp-options := $(out_root)/libcommon/common/common.l.cpp-options

driver  := $(out_base)/driver
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

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

genf := $(foreach f,$(odb_hdr:.hxx=),$(addprefix $f,-odb.hxx -odb.ixx -odb.cxx .sql))
gen  := $(addprefix $(out_base)/,$(genf))

$(gen): $(odb)
$(gen): odb := $(odb)
$(gen) $(dist): export odb_options += --generate-schema --generate-query \
--generate-prepared --table-prefix inhrt_p_
$(gen): cpp_options := -I$(src_base)
$(gen): $(common.l.cpp-options)

$(gen): odb_options += --database $(db_id)

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
$(dist): export odb_header_stem := $(basename $(odb_hdr))
$(dist): export extra_dist := $(data_dist) $(call vc9projs,$(name)) \
$(call vc10projs,$(name)) $(call vc11projs,$(name))
$(dist):
	$(call dist-data,$(sources) $(headers) $(data_dist))
	$(call meta-automake,../../template/Makefile.am)
	$(call meta-vc9projs,../../template/template,$(name))
	$(call meta-vc10projs,../../template/template,$(name))
	$(call meta-vc11projs,../../template/template,$(name))

# Test.
#
$(test): schemas := $(addprefix $(out_base)/,$(odb_hdr:.hxx=.sql))
$(test): $(driver) $(src_base)/test.std
	$(call schema,$(schemas))
	$(call message,test $<,$< --options-file $(dcf_root)/$(db_id).options \
>$(out_base)/test.out)
	$(call message,,diff -u $(src_base)/test.std $(out_base)/test.out)
	$(call message,,rm -f $(out_base)/test.out)

# Clean.
#
$(clean):                            \
  $(driver).o.clean                  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))  \
  $(addprefix $(out_base)/,$(odb_hdr:.hxx=-odb.cxx.hxx.clean))
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
$(call include,$(bld_root)/meta/vc9proj.make)
$(call include,$(bld_root)/meta/vc10proj.make)
$(call include,$(bld_root)/meta/vc11proj.make)
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
