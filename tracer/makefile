# file      : tracer/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=      \
template      \
object        \
transaction   \
types

default   := $(out_base)/
dist      := $(out_base)/.dist
test      := $(out_base)/.test
clean     := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(tests)))

$(dist): export dirs := $(tests)
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(tests)))
	$(call meta-automake)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(tests)))

$(call include,$(bld_root)/meta/automake.make)

$(foreach t,$(tests),$(call import,$(src_base)/$t/makefile))
