# file      : qt/mysql/makefile
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

tests :=   \
basic      \
date-time  \
template

default   := $(out_base)/
dist      := $(out_base)/.dist
test      := $(out_base)/.test
clean     := $(out_base)/.clean

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(tests)))

$(dist): name := qt-mysql
$(dist): export dirs := $(tests)
$(dist): export extra_dist := $(name)-vc9.sln $(name)-vc10.sln test.bat
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(tests)))
	$(call meta-automake)
	$(call meta-vc9sln,$(name)-vc9.sln)
	$(call meta-vc10sln,$(name)-vc10.sln)
	$(call meta-vctest,$(name)-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(tests)))

$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)

$(foreach t,$(tests),$(call import,$(src_base)/$t/makefile))
