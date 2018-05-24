# file      : odb/boost/makefile
# copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx := date-time/exceptions.cxx

cxx_tun := $(cxx)
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o))
cxx_od  := $(cxx_obj:.o=.o.d)

odb_boost.l             := $(out_base)/odb-boost.l
odb_boost.l.cpp-options := $(out_base)/odb-boost.l.cpp-options

default := $(out_base)/
dist    := $(out_base)/.dist
clean   := $(out_base)/.clean

# Import.
#
$(call import,\
  $(scf_root)/import/libodb/stub.make,\
  l: odb.l,cpp-options: odb.l.cpp-options)

# Build.
#
$(odb_boost.l): $(cxx_obj) $(odb.l)
$(odb_boost.l.cpp-options): value := -I$(out_root) -I$(src_root)
$(odb_boost.l.cpp-options): $(odb.l.cpp-options)

$(cxx_obj) $(cxx_od): $(odb_boost.l.cpp-options) $(out_base)/details/config.h

$(out_base)/details/config.h: | $(out_base)/details/.
	@echo '/* file      : odb/boost/details/config.h'               >$@
	@echo ' * note      : automatically generated'                  >>$@
	@echo ' */'                                                     >>$@
	@echo ''                                                        >>$@
	@echo '#ifndef ODB_BOOST_DETAILS_CONFIG_H'                      >>$@
	@echo '#define ODB_BOOST_DETAILS_CONFIG_H'                      >>$@
	@echo ''                                                        >>$@
	@echo ''                                                        >>$@
	@echo '#endif /* ODB_BOOST_DETAILS_CONFIG_H */'                 >>$@

$(call include-dep,$(cxx_od),$(cxx_obj),$(out_base)/details/config.h)

# Convenience alias for default target.
#
$(out_base)/: $(odb_boost.l)

# Dist.
#
$(dist): export sources := $(cxx)
$(dist): export headers = $(subst $(src_base)/,,$(shell find $(src_base) \
-name '*.hxx' -o -name '*.ixx' -o -name '*.txx'))
$(dist): export options = $(subst $(src_base)/,,$(shell find $(src_base) \
-name '*.options'))
$(dist): export extra_dist := libodb-boost-vc8.vcproj libodb-boost-vc9.vcproj \
libodb-boost-vc10.vcxproj libodb-boost-vc10.vcxproj.filters \
libodb-boost-vc11.vcxproj libodb-boost-vc11.vcxproj.filters \
libodb-boost-vc12.vcxproj libodb-boost-vc12.vcxproj.filters
$(dist): export interface_version = $(shell sed -e \
's/^\([0-9]*\.[0-9]*\).*/\1/' $(src_root)/version)

$(dist):
	$(call dist-data,$(sources) $(headers) $(options) details/config.h.in)
	$(call meta-vc8proj,$(src_base)/libodb-boost-vc8.vcproj)
	$(call meta-vc9proj,$(src_base)/libodb-boost-vc9.vcproj)
	$(call meta-vc10proj,$(src_base)/libodb-boost-vc10.vcxproj)
	$(call meta-vc11proj,$(src_base)/libodb-boost-vc11.vcxproj)
	$(call meta-vc12proj,$(src_base)/libodb-boost-vc12.vcxproj)
	$(call meta-automake)

# Clean.
#
$(clean): $(odb_boost.l).o.clean    \
  $(odb_boost.l.cpp-options).clean  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))
	$(call message,rm $$1,rm -f $$1,$(out_base)/details/config.h)

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(odb_boost.l): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := details/config.h
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

$(call include,$(bld_root)/cxx/o-l.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/cxx-d.make)
