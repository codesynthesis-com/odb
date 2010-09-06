# file      : tracer/transaction/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx_tun := driver.cxx
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o))
cxx_od  := $(cxx_obj:.o=.o.d)

driver  := $(out_base)/driver
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

# Import.
#
$(call import,\
  $(scf_root)/import/libodb/stub.make,\
  l: odb.l,cpp-options: odb.l.cpp-options)

$(call import,\
  $(scf_root)/import/libodb-tracer/stub.make,\
  l: odb_tracer.l,cpp-options: odb_tracer.l.cpp-options)

# Build.
#
$(driver): $(cxx_obj) $(odb_tracer.l) $(odb.l)
$(cxx_obj) $(cxx_od): cpp_options := -I$(out_base)
$(cxx_obj) $(cxx_od): $(odb_tracer.l.cpp-options) $(odb.l.cpp-options)

$(call include-dep,$(cxx_od))

# Alias for default target.
#
$(out_base)/: $(driver)

# Dist
#
$(dist): data_dist := $(cxx_tun) test.std
$(dist):
	$(call dist-data,$(data_dist))
	$(call meta-automake,$(src_root)/tracer/template/Makefile.am)

# Test.
#
$(test): $(driver) $(src_base)/test.std
	$(call message,test $<,$< | diff -u $(src_base)/test.std -)

# Clean.
#
$(clean):                            \
  $(driver).o.clean                  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))

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
$(call include,$(bld_root)/meta/automake.make)

$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-e.make)
