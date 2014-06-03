# file      : qt/mssql/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

tests :=   \
basic      \
date-time  \
template

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(tests)))

$(dist): name := qt-mssql
$(dist): export dirs := $(tests)
$(dist): export extra_dist := test.bat \
$(name)-qt4-vc8.sln \
$(name)-qt4-vc9.sln $(name)-qt5-vc9.sln \
$(name)-qt4-vc10.sln $(name)-qt5-vc10.sln \
$(name)-qt4-vc11.sln $(name)-qt5-vc11.sln \
$(name)-qt4-vc12.sln $(name)-qt5-vc12.sln
$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(tests)))
	$(call meta-automake)
	$(call meta-vc8sln,$(name)-qt4-vc8.sln,,-qt4-vc8.vcproj)
	$(call meta-vc9sln,$(name)-qt4-vc9.sln,,-qt4-vc9.vcproj)
	$(call meta-vc9sln,$(name)-qt5-vc9.sln,,-qt5-vc9.vcproj)
	$(call meta-vc10sln,$(name)-qt4-vc10.sln,,-qt4-vc10.vcxproj)
	$(call meta-vc10sln,$(name)-qt5-vc10.sln,,-qt5-vc10.vcxproj)
	$(call meta-vc11sln,$(name)-qt4-vc11.sln,,-qt4-vc11.vcxproj)
	$(call meta-vc11sln,$(name)-qt5-vc11.sln,,-qt5-vc11.vcxproj)
	$(call meta-vc12sln,$(name)-qt4-vc12.sln,,-qt4-vc12.vcxproj)
	$(call meta-vc12sln,$(name)-qt5-vc12.sln,,-qt5-vc12.vcxproj)
        # Can't have Qt-version specific tests.
	$(call meta-vctest,$(name)-qt4-vc10.sln,test.bat)

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
