# file      : evolution/template/makefile
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx_tun := driver.cxx
odb_hdr := test1.hxx test2.hxx test3.hxx
genf1   := test1-odb.hxx test1-odb.ixx test1-odb.cxx
gen1    := $(addprefix $(out_base)/,$(genf1))
genf2   := test2-odb.hxx test2-odb.ixx test2-odb.cxx
gen2    := $(addprefix $(out_base)/,$(genf2))
genf3   := test3-odb.hxx test3-odb.ixx test3-odb.cxx
gen3    := $(addprefix $(out_base)/,$(genf3))
genf    := $(genf1) $(genf2) $(genf3)
gen     := $(gen1) $(gen2) $(gen3)
gens    := test1.sql test2.sql test3.sql test3-002-pre.sql test3-002-post.sql \
test3-003-pre.sql test3-003-post.sql
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
$(gen) $(dist): odb_common_options = --generate-query \
--generate-schema --at-once --table-prefix evo_template_ #@@ CHANGE THIS
$(gen): odb_common_options += --database $(db_id)
$(gen1) $(dist): export odb_options1 = $(odb_common_options) --init-changelog
$(gen2) $(dist): export odb_options2 = $(odb_common_options) --omit-create \
--suppress-migration
$(gen3) $(dist): export odb_options3 = $(odb_common_options) --omit-create
$(gen1): odb_options += $(odb_options1) --changelog $(out_base)/model.xml
$(gen2): odb_options += $(odb_options2) --changelog $(out_base)/model.xml
$(gen3): odb_options += $(odb_options3) --changelog $(out_base)/model.xml
$(gen): cpp_options := -I$(src_base)
$(gen): $(common.l.cpp-options)

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Make sure testN.hxx are compiled serially since they share the
# changelog. Also add dependency on model.hxx
#
$(gen2): $(gen1)
$(gen3): $(gen2)
$(gen): $(src_base)/model.hxx

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
name := $(subst /,-,$(subst $(src_root)/evolution/,,$(src_base)))

$(dist): sources := $(cxx_tun)
$(dist): headers := $(odb_hdr)
$(dist): export extra_headers := model.hxx
$(dist): export name := $(name)
$(dist): export extra_dist := $(call vc8projs,$(name)) \
$(call vc9projs,$(name)) $(call vc10projs,$(name)) $(call vc11projs,$(name)) \
$(call vc12projs,$(name))
$(dist):
	$(call dist-data,$(sources) $(headers) $(extra_headers))
	$(call meta-automake,../template/Makefile.am)
	$(call meta-vc8projs,../template/template,$(name))
	$(call meta-vc9projs,../template/template,$(name))
	$(call meta-vc10projs,../template/template,$(name))
	$(call meta-vc11projs,../template/template,$(name))
	$(call meta-vc12projs,../template/template,$(name))

# Test.
#
$(test): $(driver)
        # Drop everything.
	$(call schema,$(out_base)/test3.sql)
	$(call schema,$(out_base)/test2.sql)
	$(call schema,$(out_base)/test1.sql)
        # Base schema.
	$(call schema,$(out_base)/test3-002-pre.sql)
	$(call schema,$(out_base)/test3-002-post.sql)
	$(call message,test $< base,$< --options-file $(dcf_root)/$(db_id).options 1)
        # Migration.
	$(call schema,$(out_base)/test3-003-pre.sql)
	$(call message,test $< migration,$< --options-file $(dcf_root)/$(db_id).options 2)
	$(call schema,$(out_base)/test3-003-post.sql)
        # Current schema.
	$(call message,test $< current,$< --options-file $(dcf_root)/$(db_id).options 3)

# Clean.
#
$(clean):                            \
  $(driver).o.clean                  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))  \
  $(addsuffix .hxx.clean,$(filter %.cxx,$(gen)))
	$(call message,,rm -f $(out_base)/model.xml)   # Changelog.
	$(call message,,rm -f $(out_base)/test3-*.sql) # Migration files.

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(driver): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := driver model.xml $(genf) $(gens)
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
