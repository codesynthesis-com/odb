# file      : common/changelog/makefile
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../../build/bootstrap.make

odb_hdr :=           \
model.hxx            \
add-table.hxx        \
drop-table.hxx       \
add-column.hxx       \
drop-column.hxx      \
alter-column.hxx     \
add-index.hxx        \
drop-index.hxx       \
add-foreign-key.hxx  \
drop-foreign-key.hxx 
genf := $(call odb-gen,$(odb_hdr))
gen  := $(addprefix $(out_base)/,$(genf))

common.l.cpp-options := $(out_root)/libcommon/common/common.l.cpp-options

# Import.
#
$(call import,\
  $(scf_root)/import/odb/stub.make,\
  odb: odb,odb-rules: odb_rules)

# Build.
#
$(default):

$(gen): $(odb) FORCE # Force regeneration even if up to date.
$(gen): odb := $(odb)
$(gen): export odb_options += --database $(db_id) --generate-schema-only \
--schema-format sql --suppress-migration --changelog-dir $(out_base)
$(gen): cpp_options := -I$(src_base)
$(gen): $(common.l.cpp-options)

ifdef STEP

ifeq ($(STEP),1)
$(gen): cpp_options += -DBVER=1 -DCVER=1 # Initialize.
$(gen): odb_options += --init-changelog  # Suppress notice.
else ifeq ($(STEP),2)
$(gen): cpp_options += -DBVER=1 -DCVER=2 # Diff.
else ifeq ($(STEP),3)
$(gen): cpp_options += -DBVER=2 -DCVER=3 # Patch (via rewind).
else
$(error unexpected STEP value $(STEP)
endif

$(default): $(gen)
else
$(default):
endif

# Dist: not supported.
#
$(dist):

# Test.
#
$(test): tests := $(odb_hdr:.hxx=)
$(test): diff = $(call message,,diff -u $(src_base)/$1-$(db_id)-$2.xml \
$(out_base)/$1.xml)$(literal_newline)$(literal_tab)
$(test): | $(out_base)/.
	$(call message,,rm -f $(addprefix $(out_base)/,$(addsuffix .xml,$(tests))))
	$(call message,,$(MAKE) --no-print-directory -C $(out_base) -f $(src_base)/makefile STEP=1)
	$(call message,,$(MAKE) --no-print-directory -C $(out_base) -f $(src_base)/makefile STEP=2)
	$(foreach t,$(tests),$(call diff,$t,diff))
	$(call message,,$(MAKE) --no-print-directory -C $(out_base) -f $(src_base)/makefile STEP=3)
	$(foreach t,$(tests),$(call diff,$t,patch))

# Clean.
#
$(clean): changelogs := $(addprefix $(out_base)/,$(odb_hdr:.hxx=.xml))
$(clean): $(addsuffix .hxx.clean,$(filter %.cxx,$(gen)))
	$(call message,,rm -f $(changelogs))

# Generated .gitignore.
#
ifeq ($(out_base),$(src_base))
$(default) $(test): | $(out_base)/.gitignore

$(out_base)/.gitignore: files := $(genf)
$(clean): $(out_base)/.gitignore.clean

$(call include,$(bld_root)/git/gitignore.make)
endif

# How to.
#
$(call include,$(bld_root)/cxx/standard.make) # cxx_standard
ifdef cxx_standard
$(gen): odb_options += --std $(cxx_standard)
$(call include,$(odb_rules))
endif

# Dependencies.
#
$(call import,$(src_root)/libcommon/makefile)
