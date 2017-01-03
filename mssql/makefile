# file      : mssql/makefile
# copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=      \
template      \
custom        \
database      \
native        \
query         \
stored-proc   \
types

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(tests)))

$(dist): name := mssql
$(dist): export dirs := $(tests)
$(dist): export extra_dist := $(name)-vc8.sln $(name)-vc9.sln \
$(name)-vc10.sln $(name)-vc11.sln $(name)-vc12.sln test.bat
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(tests)))
	$(call meta-automake)
	$(call meta-vc8sln1,$(name))
	$(call meta-vc9sln1,$(name))
	$(call meta-vc10sln1,$(name))
	$(call meta-vc11sln1,$(name))
	$(call meta-vc12sln1,$(name))
	$(call meta-vctest,$(name)-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(tests)))

$(call include,$(bld_root)/meta/vc8sln.make)
$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vc11sln.make)
$(call include,$(bld_root)/meta/vc12sln.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)

$(foreach t,$(tests),$(call import,$(src_base)/$t/makefile))
