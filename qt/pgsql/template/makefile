# file      : qt/pgsql/template/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
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

$(call import,\
  $(scf_root)/import/libodb-qt/stub.make,\
  l: odb_qt.l,cpp-options: odb_qt.l.cpp-options)

#Build.
#
$(driver): $(cxx_obj) $(odb_qt.l) $(common.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base) -I$(src_base)
$(cxx_obj) $(cxx_od): $(common.l.cpp-options) $(odb_qt.l.cpp-options)

$(gen): $(odb)
$(gen): odb := $(odb)
$(gen) $(dist): export odb_options += --database pgsql --profile qt \
--generate-schema --table-prefix qt_pgsql_template_ #@@ CHANGE THIS
$(gen): cpp_options := -I$(src_base)
$(gen): $(common.l.cpp-options) $(odb_qt.l.cpp-options)

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
$(dist): sources := $(cxx_tun)
$(dist): headers := $(odb_hdr)
$(dist): data_dist := test.std
$(dist): export name := $(subst /,-,$(subst $(src_root)/qt/pgsql/,,$(src_base)))
$(dist): export extra_dist := $(data_dist) \
$(name)-qt4-vc8.vcproj \
$(name)-qt4-vc9.vcproj $(name)-qt5-vc9.vcproj \
$(name)-qt4-vc10.vcxproj $(name)-qt4-vc10.vcxproj.filters \
$(name)-qt5-vc10.vcxproj $(name)-qt5-vc10.vcxproj.filters \
$(name)-qt4-vc11.vcxproj $(name)-qt4-vc11.vcxproj.filters \
$(name)-qt5-vc11.vcxproj $(name)-qt5-vc11.vcxproj.filters \
$(name)-qt4-vc12.vcxproj $(name)-qt4-vc12.vcxproj.filters \
$(name)-qt5-vc12.vcxproj $(name)-qt5-vc12.vcxproj.filters
$(dist):
	$(call dist-data,$(sources) $(headers) $(data_dist))
	$(call meta-automake,../template/Makefile.am)
	$(call meta-vc8proj,../template/template-qt4-vc8.vcproj,$(name)-qt4-vc8.vcproj)
	$(call meta-vc9proj,../template/template-qt4-vc9.vcproj,$(name)-qt4-vc9.vcproj)
	$(call meta-vc9proj,../template/template-qt5-vc9.vcproj,$(name)-qt5-vc9.vcproj)
	$(call meta-vc10proj,../template/template-qt4-vc10.vcxproj,$(name)-qt4-vc10.vcxproj)
	$(call meta-vc10proj,../template/template-qt5-vc10.vcxproj,$(name)-qt5-vc10.vcxproj)
	$(call meta-vc11proj,../template/template-qt4-vc11.vcxproj,$(name)-qt4-vc11.vcxproj)
	$(call meta-vc11proj,../template/template-qt5-vc11.vcxproj,$(name)-qt5-vc11.vcxproj)
	$(call meta-vc12proj,../template/template-qt4-vc12.vcxproj,$(name)-qt4-vc12.vcxproj)
	$(call meta-vc12proj,../template/template-qt5-vc12.vcxproj,$(name)-qt5-vc12.vcxproj)

# Test.
#
$(eval $(call test-rule))

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
