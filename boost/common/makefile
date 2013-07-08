# file      : boost/common/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

tests :=      \
multi-index   \
optional      \
smart-ptr     \
template      \
unordered     \
uuid

all_tests := $(tests)
build_tests := $(tests)

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

name := boost-common
$(dist): name := $(name)
$(dist): export dirs := $(tests)
$(dist): export extra_dist :=  test.bat $(call vc8slns,$(name)) \
$(call vc9slns,$(name)) $(call vc10slns,$(name)) $(call vc11slns,$(name))
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call meta-automake)
	$(call meta-vc8slns,$(name))
	$(call meta-vc9slns,$(name))
	$(call meta-vc10slns,$(name))
	$(call meta-vc11slns,$(name))
	$(call meta-vctest,$(name)-mysql-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_tests)))

ifeq ($(db_id),common)
$(foreach d,$(databases),$(eval $(call db-test-dir,$d,$(tests))))
endif

$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_tests)))

$(call include,$(bld_root)/meta/vc8sln.make)
$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vc11sln.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach t,$(all_tests),$(call import,$(src_base)/$t/makefile))
else
$(foreach t,$(build_tests),$(call import,$(src_base)/$t/makefile))
endif
