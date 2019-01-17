# file      : hello/makefile
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

cxx_tun := driver.cxx
odb_hdr := person.hxx
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o) $(odb_hdr:.hxx=-odb.o))
cxx_od  := $(cxx_obj:.o=.o.d)

driver  := $(out_base)/driver
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

# Import.
#
$(call import,\
  $(scf_root)/import/odb/stub.make,\
  odb: odb,odb-rules: odb_rules)

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
$(driver): $(cxx_obj) $(odb_db.l) $(odb.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base) -I$(src_base) -D$(db_macro)
$(cxx_obj) $(cxx_od): $(odb.l.cpp-options) $(odb_db.l.cpp-options)

genf := $(addprefix $(odb_hdr:.hxx=-odb),.hxx .ixx .cxx) $(odb_hdr:.hxx=.sql)
gen  := $(addprefix $(out_base)/,$(genf))

$(gen): $(odb)
$(gen): odb := $(odb)
$(gen) $(dist): export odb_options += --generate-query --generate-schema
$(gen): cpp_options := -I$(src_base)
$(gen): $(odb.l.cpp-options)

$(gen): odb_options += --database $(db_id)

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
name := $(subst /,-,$(subst $(src_root)/,,$(src_base)))

$(dist): sources := $(cxx_tun)
$(dist): headers := $(odb_hdr)
$(dist): export name := $(name)
$(dist): export odb_header_stem := $(basename $(odb_hdr))
$(dist): export extra_dist := README $(call vc8projs,$(name)) \
$(call vc9projs,$(name)) $(call vc10projs,$(name)) $(call vc11projs,$(name)) \
$(call vc12projs,$(name))
$(dist):
	$(call dist-data,$(sources) $(headers) README database.hxx)
	$(call meta-automake,../template/Makefile.am)
	$(call meta-vc8projs,../template/template,$(name))
	$(call meta-vc9projs,../template/template,$(name))
	$(call meta-vc10projs,../template/template,$(name))
	$(call meta-vc11projs,../template/template,$(name))
	$(call meta-vc12projs,../template/template,$(name))

# Test.
#
$(test): header := $(odb_hdr)
$(test): $(driver)
	$(call schema)
	$(call message,test $<,$< --options-file $(dcf_root)/$(db_id).options)

# Clean.
#
$(clean):                            \
  $(driver).o.clean                  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))  \
  $(addprefix $(out_base)/,$(odb_hdr:.hxx=-odb.cxx.hxx.clean))

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

