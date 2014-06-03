# file      : common/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

tests :=                  \
access                    \
auto                      \
blob                      \
callback                  \
changelog                 \
circular/single           \
circular/multiple         \
composite                 \
composite-id              \
const-object              \
const-member              \
container/basics          \
container/change-tracking \
ctor                      \
default                   \
definition                \
enum                      \
erase-query               \
include                   \
index                     \
inheritance/polymorphism  \
inheritance/reuse         \
inheritance/transient     \
inverse                   \
lazy-ptr                  \
lifecycle                 \
no-id                     \
optimistic                \
pragma                    \
prepared                  \
query/basics              \
query/array               \
readonly                  \
relationship/basics       \
relationship/on-delete    \
relationship/query        \
schema/namespace          \
schema/embedded/basics    \
schema/embedded/order     \
section/basics            \
section/polymorphism      \
session/cache             \
statement/processing      \
template                  \
transaction/basics        \
transaction/callback      \
types                     \
view                      \
virtual                   \
wrapper

thread_tests := threads
cxx11_tests  := session/custom

no_dist_tests := changelog include

no_multi_tests := changelog include

$(default):
$(call include,$(bld_root)/cxx/standard.make) # cxx_standard

all_tests := $(tests) $(thread_tests) $(cxx11_tests)
build_tests := $(tests) $(thread_tests)

ifeq ($(cxx_standard),c++11)
build_tests += $(cxx11_tests)
endif

ifeq ($(db_id),common)
build_tests := $(filter-out $(no_multi_tests),$(build_tests))
endif

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_tests)))

name := common
$(dist): name := $(name)
$(dist): export dirs := $(filter-out $(no_dist_tests),$(tests))
$(dist): export thread_dirs := $(thread_tests)
$(dist): export cxx11_dirs := $(cxx11_tests)
$(dist): export extra_dist := test.bat $(call vc8slns,$(name)) \
$(call vc9slns,$(name)) $(call vc10slns,$(name)) $(call vc11slns,$(name)) \
$(call vc12slns,$(name))
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_tests)))
	$(call meta-automake)
	$(call meta-vc8slns,$(name))
	$(call meta-vc9slns,$(name))
	$(call meta-vc10slns,$(name))
	$(call meta-vc11slns,$(name))
	$(call meta-vc12slns,$(name))
	$(call meta-vctest,$(name)-mysql-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_tests)))

ifeq ($(db_id),common)
$(foreach d,$(databases),$(eval $(call db-test-dir,$d,$(build_tests))))
endif

$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_tests)))

$(call include,$(bld_root)/meta/vc8sln.make)
$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vc11sln.make)
$(call include,$(bld_root)/meta/vc12sln.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach t,$(all_tests),$(call import,$(src_base)/$t/makefile))
else
$(foreach t,$(build_tests),$(call import,$(src_base)/$t/makefile))
endif
