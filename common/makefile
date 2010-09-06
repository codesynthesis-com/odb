# file      : common/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=      \
auto          \
schema        \
template      \
lifecycle     \
query

thread_tests := threads

all_tests := $(tests) $(thread_tests)
build_tests := $(tests) $(thread_tests)

default   := $(out_base)/
dist      := $(out_base)/.dist
test      := $(out_base)/.test
clean     := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

$(dist): export dirs := $(tests)
$(dist): export thread_dirs := $(thread_tests)
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call meta-automake)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_tests)))

$(call include,$(bld_root)/dist.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach t,$(all_tests),$(call import,$(src_base)/$t/makefile))
else
$(foreach t,$(build_tests),$(call import,$(src_base)/$t/makefile))
endif
