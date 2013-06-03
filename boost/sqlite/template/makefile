# file      : boost/sqlite/template/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../../build/bootstrap.make

cxx_tun := driver.cxx
odb_hdr := test.hxx
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

$(call import,\
  $(scf_root)/import/libodb-boost/stub.make,\
  l: odb_boost.l,cpp-options: odb_boost.l.cpp-options)

$(call import,\
  $(scf_root)/import/libboost/header-only/stub.make,\
  cpp-options: boost.l.cpp-options)

$(call import,\
  $(scf_root)/import/libboost/date-time/stub.make,\
  l: boost_date_time.l)

# Build.
#
$(driver): $(cxx_obj) $(odb_boost.l) $(common.l) $(boost_date_time.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base) -I$(src_base)
$(cxx_obj) $(cxx_od): $(common.l.cpp-options) $(odb_boost.l.cpp-options) \
$(boost.l.cpp-options)

genf := $(addprefix $(odb_hdr:.hxx=-odb),.hxx .ixx .cxx) $(odb_hdr:.hxx=.sql)
gen  := $(addprefix $(out_base)/,$(genf))

$(gen): $(odb)
$(gen): odb := $(odb)
$(gen) $(dist): export odb_options += --database sqlite --profile boost \
--generate-schema --table-prefix boost_sqlite_template_ #@@ CHANGE THIS
$(gen): cpp_options := -I$(src_base)
$(gen): $(common.l.cpp-options) $(odb_boost.l.cpp-options) \
$(boost.l.cpp-options)

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
$(dist): sources := $(cxx_tun)
$(dist): headers := $(odb_hdr)
$(dist): data_dist := test.std
$(dist): export name := $(subst /,-,$(subst $(src_root)/boost/sqlite/,,$(src_base)))
$(dist): export extra_dist := $(data_dist) $(name)-vc9.vcproj \
$(name)-vc10.vcxproj $(name)-vc10.vcxproj.filters \
$(name)-vc11.vcxproj $(name)-vc11.vcxproj.filters
$(dist):
	$(call dist-data,$(sources) $(headers) $(data_dist))
	$(call meta-automake,../template/Makefile.am)
	$(call meta-vc9proj,../template/template-vc9.vcproj,$(name)-vc9.vcproj)
	$(call meta-vc10proj,../template/template-vc10.vcxproj,$(name)-vc10.vcxproj)
	$(call meta-vc11proj,../template/template-vc11.vcxproj,$(name)-vc11.vcxproj)

# Test.
#
$(test): $(driver) $(src_base)/test.std
	$(call schema)
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
