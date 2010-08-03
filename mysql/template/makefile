# file      : mysql/template/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx_tun := driver.cxx
odb_hdr := test.hxx
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o) $(odb_hdr:.hxx=-odb.o))
cxx_od  := $(cxx_obj:.o=.o.d)

common.l             := $(out_root)/libcommon/common.l
common.l.cpp-options := $(out_root)/libcommon/common.l.cpp-options

driver  := $(out_base)/driver
test    := $(out_base)/.test
clean   := $(out_base)/.clean

ifdef db_id
ifneq ($(db_id),mysql)
$(error trying to build a MySQL-specific test with $(db_id))
endif
endif

# Import.
#
$(call import,\
  $(scf_root)/import/odb/stub.make,\
  odb: odb,odb-rules: odb_rules)

# Build.
#
$(driver): $(cxx_obj) $(common.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base)
$(cxx_obj) $(cxx_od): $(common.l.cpp-options)

genf := $(addprefix $(odb_hdr:.hxx=-odb),.hxx .ixx .cxx) $(odb_hdr:.hxx=.sql)
gen  := $(addprefix $(out_base)/,$(genf))

$(gen): $(odb)
$(gen): odb := $(odb)
$(gen): odb_options += --database $(db_id) --generate-schema
$(gen): cpp_options := -I$(out_base)
$(gen): $(common.l.cpp-options)

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Alias for default target.
#
$(out_base)/: $(driver)

# Test.
#
$(test): $(driver) $(src_base)/test.std
	$(call message,sql $$1,$(dcf_root)/db-driver $$1, $(src_base)/test.sql)
	$(call message,test $<,$< --options-file $(dcf_root)/db.options \
| diff -u $(src_base)/test.std -)

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
$(call include,$(odb_rules))
$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-e.make)

# Dependencies.
#
$(call import,$(src_root)/libcommon/makefile)
