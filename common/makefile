# file      : common/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=      \
auto          \
composite     \
const         \
container     \
ctor          \
enum          \
inverse       \
lazy-ptr      \
lifecycle     \
query         \
relationship  \
schema        \
template

thread_tests := threads

all_tests := $(tests) $(thread_tests)
build_tests := $(tests) $(thread_tests)

default   := $(out_base)/
dist      := $(out_base)/.dist
test      := $(out_base)/.test
clean     := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

name := common
$(dist): name := $(name)
$(dist): export dirs := $(tests)
$(dist): export thread_dirs := $(thread_tests)
$(dist): export extra_dist :=  test.bat $(call vc9slns,$(name)) \
$(call vc10slns,$(name))
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call meta-automake)
	$(call meta-vc9slns,$(name))
	$(call meta-vc10slns,$(name))
	$(call meta-vctest,$(name)-mysql-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_tests)))

$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach t,$(all_tests),$(call import,$(src_base)/$t/makefile))
else
$(foreach t,$(build_tests),$(call import,$(src_base)/$t/makefile))
endif
