# file      : libcommon/makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))../build/bootstrap.make

# Options file.
#
cli_tun := options.cli
cxx_tun := common.cxx

cxx_obj   := $(addprefix $(out_base)/,$(cxx_tun:.cxx=.o) $(cli_tun:.cli=.o))
cxx_od    := $(cxx_obj:.o=.o.d)

common.l             := $(out_base)/common.l
common.l.cpp-options := $(out_base)/common.l.cpp-options

default   := $(out_base)/
clean     := $(out_base)/.clean

# Import.
#
$(call import,\
  $(scf_root)/import/cli/stub.make,\
  cli: cli,cli-rules: cli_rules)

$(call import,\
  $(scf_root)/import/libodb/stub.make,\
  l: odb.l,cpp-options: odb.l.cpp-options)

ifdef db_id
$(call import,\
  $(scf_root)/import/libodb-$(db_id)/stub.make,\
  l: odb_db.l,cpp-options: odb_db.l.cpp-options)
endif

ifeq ($(odb_db.l.cpp-options),)
odb_db.l.cpp-options := $(out_base)/.unbuildable
endif

# Build.
#
$(common.l): $(cxx_obj) $(odb.l) $(odb_db.l)

$(cxx_obj) $(cxx_od): $(common.l.cpp-options)
$(common.l.cpp-options): value := -I$(src_base)
$(common.l.cpp-options): $(odb_db.l.cpp-options) $(odb.l.cpp-options)

ifeq ($(db_id),mysql)
$(cxx_obj): cpp_options += -DDB_ID_MYSQL
endif

genf := $(cli_tun:.cli=.hxx) $(cli_tun:.cli=.ixx) $(cli_tun:.cli=.cxx)
gen  := $(addprefix $(out_base)/,$(genf))

$(gen): $(cli)
$(gen): cli := $(cli)
$(gen): cli_options +=   \
--generate-specifier     \
--generate-file-scanner  \
--guard-prefix LIBCOMMON

$(call include-dep,$(cxx_od),$(cxx_obj),$(gen))

# Convenience alias for default target.
#
$(out_base)/: $(common.l)

# Clean.
#
$(clean): $(common.l).o.clean        \
  $(common.l.cpp-options).clean      \
  $(addsuffix .cxx.clean,$(cxx_obj)) \
  $(addsuffix .cxx.clean,$(cxx_od))

# How to.
#
$(call include,$(cli_rules))
$(call include,$(bld_root)/cxx/cxx-d.make)
$(call include,$(bld_root)/cxx/cxx-o.make)
$(call include,$(bld_root)/cxx/o-l.make)
