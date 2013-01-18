# file      : common/makefile
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=                 \
access                   \
auto                     \
blob                     \
callback                 \
circular/single          \
circular/multiple        \
composite                \
composite-id             \
const-object             \
const-member             \
container                \
ctor                     \
default                  \
definition               \
enum                     \
erase-query              \
include                  \
index                    \
inheritance/polymorphism \
inheritance/reuse        \
inheritance/transient    \
inverse                  \
lazy-ptr                 \
lifecycle                \
no-id                    \
optimistic               \
pragma                   \
prepared                 \
query                    \
readonly                 \
relationship             \
relationship-query       \
schema                   \
session/cache            \
template                 \
transaction/basics       \
transaction/callback     \
types                    \
view                     \
virtual                  \
wrapper

thread_tests := threads
cxx11_tests  := session/custom

default   := $(out_base)/
dist      := $(out_base)/.dist
test      := $(out_base)/.test
clean     := $(out_base)/.clean

$(default):
$(call include,$(bld_root)/cxx/standard.make) # cxx_standard

all_tests := $(tests) $(thread_tests) $(cxx11_tests)
build_tests := $(tests) $(thread_tests)

ifeq ($(cxx_standard),c++11)
build_tests += $(cxx11_tests)
endif

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

name := common
$(dist): name := $(name)
$(dist): export dirs := $(filter-out include,$(tests))
$(dist): export thread_dirs := $(thread_tests)
$(dist): export cxx11_dirs := $(cxx11_tests)
$(dist): export extra_dist :=  test.bat $(call vc9slns,$(name)) \
$(call vc10slns,$(name)) $(call vc11slns,$(name))
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call meta-automake)
	$(call meta-vc9slns,$(name))
	$(call meta-vc10slns,$(name))
	$(call meta-vc11slns,$(name))
	$(call meta-vctest,$(name)-mysql-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_tests)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_tests)))

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
