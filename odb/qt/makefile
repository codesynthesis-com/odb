# file      : odb/qt/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

cxx := date-time/exceptions.cxx

cxx_tun := $(cxx)
cxx_obj := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o))
cxx_od  := $(cxx_obj:.o=.o.d)

odb_qt.l             := $(out_base)/odb-qt.l
odb_qt.l.cpp-options := $(out_base)/odb-qt.l.cpp-options

default := $(out_base)/
dist    := $(out_base)/.dist
clean   := $(out_base)/.clean

# Import.
#
$(call import,\
  $(scf_root)/import/libodb/stub.make,\
  l: odb.l,cpp-options: odb.l.cpp-options)

$(call import,\
  $(scf_root)/import/libqt/core/stub.make,\
  l: qt_core.l,\
  cpp-options: qt_core.l.cpp-options,\
  cpp-options-inline: qt_core_cpp_options)

# Build.
#
$(odb_qt.l): $(cxx_obj) $(odb.l) $(qt_core.l)
$(odb_qt.l.cpp-options): value := -I$(out_root) -I$(src_root) $(qt_core_cpp_options)
$(odb_qt.l.cpp-options): $(odb.l.cpp-options) $(qt_core.l.cpp-options)

$(cxx_obj) $(cxx_od): $(odb_qt.l.cpp-options) $(out_base)/details/config.h

$(out_base)/details/config.h: | $(out_base)/details/.
	@echo '/* file      : odb/qt/details/config.h'                   >$@
	@echo ' * note      : automatically generated'                  >>$@
	@echo ' */'                                                     >>$@
	@echo ''                                                        >>$@
	@echo '#ifndef ODB_QT_DETAILS_CONFIG_H'                         >>$@
	@echo '#define ODB_QT_DETAILS_CONFIG_H'                         >>$@
	@echo ''                                                        >>$@
	@echo ''                                                        >>$@
	@echo '#endif /* ODB_QT_DETAILS_CONFIG_H */'                    >>$@

$(call include-dep,$(cxx_od),$(cxx_obj),$(out_base)/details/config.h)

# Convenience alias for default target.
#
$(out_base)/: $(odb_qt.l)

# Dist.
#
$(dist): export sources := $(cxx)
$(dist): export headers = $(subst $(src_base)/,,$(shell find $(src_base) \
-name '*.hxx' -o -name '*.ixx' -o -name '*.txx'))
$(dist): export options = $(subst $(src_base)/,,$(shell find $(src_base) \
-name '*.options'))
$(dist): export extra_dist := \
libodb-qt4-vc8.vcproj \
libodb-qt4-vc9.vcproj \
libodb-qt5-vc9.vcproj \
libodb-qt4-vc10.vcxproj libodb-qt4-vc10.vcxproj.filters \
libodb-qt5-vc10.vcxproj libodb-qt5-vc10.vcxproj.filters \
libodb-qt4-vc11.vcxproj libodb-qt4-vc11.vcxproj.filters \
libodb-qt5-vc11.vcxproj libodb-qt5-vc11.vcxproj.filters
$(dist): export interface_version = $(shell sed -e \
's/^\([0-9]*\.[0-9]*\).*/\1/' $(src_root)/version)

$(dist):
	$(call dist-data,$(sources) $(headers) $(options) details/config.h.in)
	$(call meta-vc8proj,$(src_base)/libodb-qt4-vc8.vcproj)
	$(call meta-vc9proj,$(src_base)/libodb-qt4-vc9.vcproj)
	$(call meta-vc9proj,$(src_base)/libodb-qt5-vc9.vcproj)
	$(call meta-vc10proj,$(src_base)/libodb-qt4-vc10.vcxproj)
	$(call meta-vc10proj,$(src_base)/libodb-qt5-vc10.vcxproj)
	$(call meta-vc11proj,$(src_base)/libodb-qt4-vc11.vcxproj)
	$(call meta-vc11proj,$(src_base)/libodb-qt5-vc11.vcxproj)
	$(call meta-automake)

# Clean.
#
$(clean): $(odb_qt.l).o.clean    \
  $(odb_qt.l.cpp-options).clean  \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))
	$(call message,rm $$1,rm -f $$1,$(out_base)/details/config.h)

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(odb_qt.l): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := details/config.h
$(clean): $(out_base)/.gitignore.clean

$(call include,$(bld_root)/git/gitignore.make)
endif

# How to.
#
$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vc8proj.make)
$(call include,$(bld_root)/meta/vc9proj.make)
$(call include,$(bld_root)/meta/vc10proj.make)
$(call include,$(bld_root)/meta/vc11proj.make)
$(call include,$(bld_root)/meta/automake.make)

$(call include,$(bld_root)/cxx/o-l.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/cxx-d.make)
