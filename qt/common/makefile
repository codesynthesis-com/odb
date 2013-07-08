# file      : qt/common/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

tests :=                   \
basic                      \
containers/basics          \
containers/change-tracking \
smart-ptr                  \
template

all_tests := $(tests)
build_tests := $(tests)

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

name := qt-common
$(dist): name := $(name)
$(dist): export dirs := $(tests)
$(dist): export extra_dist :=  test.bat \
$(call vc9slns,$(name)-qt4) \
$(call vc9slns,$(name)-qt4) $(call vc9slns,$(name)-qt5) \
$(call vc10slns,$(name)-qt4) $(call vc10slns,$(name)-qt5) \
$(call vc11slns,$(name)-qt4) $(call vc11slns,$(name)-qt5)
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call meta-automake)
	$(call meta-vc8slns,$(name)-qt4,-qt4)
	$(call meta-vc9slns,$(name)-qt4,-qt4)
	$(call meta-vc9slns,$(name)-qt5,-qt5)
	$(call meta-vc10slns,$(name)-qt4,-qt4)
	$(call meta-vc10slns,$(name)-qt5,-qt5)
	$(call meta-vc11slns,$(name)-qt4,-qt4)
	$(call meta-vc11slns,$(name)-qt5,-qt5)
        # Can't have Qt-version specific tests.
	$(call meta-vctest,$(name)-qt4-mysql-vc10.sln,test.bat)

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
