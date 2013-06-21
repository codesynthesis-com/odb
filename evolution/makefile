# file      : evolution/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=         \
add-table        \
drop-table       \
add-column       \
drop-column      \
alter-column     \
add-foreign-key  \
drop-foreign-key \
add-index        \
drop-index       \
combined         \
embedded         \
version          \
data             \
template

all_tests := $(tests)
build_tests := $(tests)

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

name := evolution
$(dist): name := $(name)
$(dist): data_dist := tester.bat
$(dist): exec_dist := tester.in
$(dist): export dirs := $(filter-out $(no_dist_tests),$(tests))
$(dist): export extra_dist := $(data_dist) $(exec_dist) test.bat \
$(call vc9slns,$(name)) $(call vc10slns,$(name)) $(call vc11slns,$(name))
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call dist-data,$(data_dist))
	$(call dist-exec,$(exec_dist))
	$(call meta-automake)
	$(call meta-vc9slns,$(name))
	$(call meta-vc10slns,$(name))
	$(call meta-vc11slns,$(name))
	$(call meta-vctest,$(name)-mysql-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_tests)))

$(call include,$(bld_root)/dist.make)
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
