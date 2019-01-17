# file      : makefile
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))build/bootstrap.make

dirs :=                  \
access                   \
composite                \
container                \
hello                    \
inheritance/polymorphism \
inheritance/reuse        \
query                    \
mapping                  \
optimistic               \
pimpl                    \
prepared                 \
schema/embedded          \
section

tr1_dirs   := relationship inverse schema/custom view
cxx11_dirs := c++11
boost_dirs := boost
qt_dirs    := qt

default := $(out_base)/
dist    := $(out_base)/.dist
test    := $(out_base)/.test
clean   := $(out_base)/.clean

$(default):
$(call include,$(bld_root)/cxx/standard.make) # cxx_standard

dist_dirs  := $(dirs) $(tr1_dirs) $(cxx11_dirs) $(boost_dirs) $(qt_dirs)
all_dirs   := $(dirs) $(tr1_dirs) $(cxx11_dirs) $(boost_dirs) $(qt_dirs) \
template

build_dirs := $(dirs) $(tr1_dirs) $(boost_dirs) $(qt_dirs) template

ifeq ($(cxx_standard),c++11)
build_dirs += $(cxx11_dirs)
endif

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(build_dirs)))

$(dist): name := examples
$(dist): export dirs := $(dirs)
$(dist): export tr1_dirs := $(tr1_dirs)
$(dist): export cxx11_dirs := $(cxx11_dirs)
$(dist): export boost_dirs := $(boost_dirs)
$(dist): export qt_dirs := $(qt_dirs)
$(dist): data_dist := GPLv2 LICENSE README NEWS INSTALL version tester.bat \
mysql-driver.bat mysql.options sqlite-driver.bat sqlite.options \
pgsql-driver.bat pgsql.options oracle-driver.bat oracle.options \
mssql-driver.bat mssql.options
$(dist): exec_dist := bootstrap tester.in
$(dist): export extra_dist := $(data_dist) $(exec_dist) test.bat \
$(call vc8slns,$(name)) $(call vc9slns,$(name)) $(call vc10slns,$(name)) \
$(call vc11slns,$(name)) $(call vc12slns,$(name))
$(dist): export version = $(shell cat $(src_root)/version)

$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(dist_dirs)))
	$(call dist-data,$(data_dist))
	$(call dist-exec,$(exec_dist))
	$(call dist-dir,m4)
	$(call meta-automake)
	$(call meta-autoconf)
	$(call meta-vc8slns,$(name))
	$(call meta-vc9slns,$(name))
	$(call meta-vc10slns,$(name))
	$(call meta-vc11slns,$(name))
	$(call meta-vc12slns,$(name))
	$(call meta-vctest,$(name)-mysql-vc10.sln,test.bat)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(build_dirs)))
$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_dirs)))

$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vc8sln.make)
$(call include,$(bld_root)/meta/vc9sln.make)
$(call include,$(bld_root)/meta/vc10sln.make)
$(call include,$(bld_root)/meta/vc11sln.make)
$(call include,$(bld_root)/meta/vc12sln.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)
$(call include,$(bld_root)/meta/autoconf.make)

$(foreach d,$(all_dirs),$(call import,$(src_base)/$d/makefile))
