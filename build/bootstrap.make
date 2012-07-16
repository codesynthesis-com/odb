# file      : build/bootstrap.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

project_name := odb-tests

# First try to include the bundled bootstrap.make if it exist. If that
# fails, let make search for the external bootstrap.make.
#
build := build-0.3

-include $(dir $(lastword $(MAKEFILE_LIST)))../../$(build)/bootstrap.make

ifeq ($(patsubst %build/bootstrap.make,,$(lastword $(MAKEFILE_LIST))),)
include $(build)/bootstrap.make
endif

# Configuration
#
$(call include,$(scf_root)/configuration.make)

def_goal := $(.DEFAULT_GOAL)

# Include C++ configuration. We need to know if we are using the generic
# C++ compiler in which case we need to compensate for missing dependency
# auto-generation (see below).
#
$(call include,$(bld_root)/cxx/configuration.make)

# Aliases
#
.PHONY: $(out_base)/       \
        $(out_base)/.test  \
        $(out_base)/.dist  \
        $(out_base)/.clean

ifdef %interactive%

.PHONY: test dist clean

test:  $(out_base)/.test
dist:  $(out_base)/.dist
clean: $(out_base)/.clean

endif

# Database schema creation.
#
ifeq ($(filter $(db_id),sqlite),)
$(out_base)/.test: schema-body = \
$(call message,sql $$1,$(dcf_root)/db-driver $$1,$1)$(literal_newline)$(literal_tab)
$(out_base)/.test: schema = \
$(foreach s,$(if $1,$1,$(out_base)/test.sql),$(call schema-body,$s))@:
endif

# Dist setup.
#
ifneq ($(filter $(MAKECMDGOALS),dist),)

# Make sure the distribution prefix is set if the goal is dist.
#
ifeq ($(dist_prefix),)
$(error dist_prefix is not set)
endif

databases := mysql sqlite pgsql oracle mssql
$(dist): databases := $(databases)

# $1 project template without the -vcN.vc[x]proj suffix.
# $2 project name without the -vcN.vc[x]proj suffix.
#
$(dist): meta-vc9projs = \
$(foreach d,$(databases),$(call \
meta-vc9proj,$1-vc9.vcproj,$(if $2,$2,$(notdir \
$1))-$d-vc9.vcproj,database,$d)$(literal_newline)$(literal_tab))@:

$(dist): meta-vc10projs = \
$(foreach d,$(databases),$(call \
meta-vc10proj,$1-vc10.vcxproj,$(if $2,$2,$(notdir \
$1))-$d-vc10.vcxproj,database,$d)$(literal_newline)$(literal_tab))@:

# $1 project name without the -vcN.vc[x]proj suffix.
#
vc9projs = $(addprefix $1-,$(addsuffix -vc9.vcproj,$(databases)))
vc10projs = $(addprefix $1-,$(addsuffix -vc10.vcxproj,$(databases))) \
$(addprefix $1-,$(addsuffix -vc10.vcxproj.filters,$(databases)))

# $1 solution name without the -vcN.sln suffix.
#
$(dist): meta-vc9slns = \
$(foreach d,$(databases),$(call \
meta-vc9sln,$1-vc9.sln,$1-$d-vc9.sln,-$d-vc9.vcproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc10slns = \
$(foreach d,$(databases),$(call \
meta-vc10sln,$1-vc10.sln,$1-$d-vc10.sln,-$d-vc10.vcxproj,database,$d)$(literal_newline)\
$(literal_tab))@:

vc9slns = $(addprefix $1-,$(addsuffix -vc9.sln,$(databases)))
vc10slns = $(addprefix $1-,$(addsuffix -vc10.sln,$(databases)))

endif

# If we don't have dependency auto-generation then we need to manually
# make sure that ODB files are generated before C++ file are compiler.
# To do this we make the object files ($2) depend in order-only on
# generated files ($3).
#
ifeq ($(cxx_id),generic)

define include-dep
$(if $2,$(eval $2: | $3))
endef

else

define include-dep
$(call -include,$1)
endef

endif

# Don't include dependency info for certain targets.
#
ifneq ($(filter $(MAKECMDGOALS),clean disfigure dist),)
include-dep =
endif

.DEFAULT_GOAL := $(def_goal)
