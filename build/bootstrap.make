# file      : build/bootstrap.make
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

project_name := odb-examples

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
$(out_base)/.test: schema = \
$(foreach h,$(header),$(call \
message,sql $$1,$(dcf_root)/$(db_id)-driver \
$$1,$(out_base)/$(basename $h).sql)$(literal_newline)$(literal_tab))
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
$(dist): meta-vc8projs = \
$(foreach d,$(databases),$(call \
meta-vc8proj,$1-vc8.vcproj,$(if $2,$2,$(notdir \
$1))-$d-vc8.vcproj,database,$d)$(literal_newline)$(literal_tab))@:

$(dist): meta-vc9projs = \
$(foreach d,$(databases),$(call \
meta-vc9proj,$1-vc9.vcproj,$(if $2,$2,$(notdir \
$1))-$d-vc9.vcproj,database,$d)$(literal_newline)$(literal_tab))@:

$(dist): meta-vc10projs = \
$(foreach d,$(databases),$(call \
meta-vc10proj,$1-vc10.vcxproj,$(if $2,$2,$(notdir \
$1))-$d-vc10.vcxproj,database,$d)$(literal_newline)$(literal_tab))@:

$(dist): meta-vc11projs = \
$(foreach d,$(databases),$(call \
meta-vc11proj,$1-vc11.vcxproj,$(if $2,$2,$(notdir \
$1))-$d-vc11.vcxproj,database,$d)$(literal_newline)$(literal_tab))@:

$(dist): meta-vc12projs = \
$(foreach d,$(databases),$(call \
meta-vc12proj,$1-vc12.vcxproj,$(if $2,$2,$(notdir \
$1))-$d-vc12.vcxproj,database,$d)$(literal_newline)$(literal_tab))@:


# $1 project name without the -vcN.vc[x]proj suffix.
#
vc8projs = $(addprefix $1-,$(addsuffix -vc8.vcproj,$(databases)))

vc9projs = $(addprefix $1-,$(addsuffix -vc9.vcproj,$(databases)))

vc10projs = $(addprefix $1-,$(addsuffix -vc10.vcxproj,$(databases))) \
$(addprefix $1-,$(addsuffix -vc10.vcxproj.filters,$(databases)))

vc11projs = $(addprefix $1-,$(addsuffix -vc11.vcxproj,$(databases))) \
$(addprefix $1-,$(addsuffix -vc11.vcxproj.filters,$(databases)))

vc12projs = $(addprefix $1-,$(addsuffix -vc12.vcxproj,$(databases))) \
$(addprefix $1-,$(addsuffix -vc12.vcxproj.filters,$(databases)))

# $1 solution name without the -vcN.sln suffix.
# $2 extra project suffix in addition to -<db>-vcN.vcproj (optional)
#
$(dist): meta-vc8slns = \
$(foreach d,$(databases),$(call \
meta-vc8sln,$(src_root)/template-vc8.sln,$1-$d-vc8.sln,$2-$d-vc8.vcproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc9slns = \
$(foreach d,$(databases),$(call \
meta-vc9sln,$(src_root)/template-vc9.sln,$1-$d-vc9.sln,$2-$d-vc9.vcproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc10slns = \
$(foreach d,$(databases),$(call \
meta-vc10sln,$(src_root)/template-vc10.sln,$1-$d-vc10.sln,$2-$d-vc10.vcxproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc11slns = \
$(foreach d,$(databases),$(call \
meta-vc11sln,$(src_root)/template-vc11.sln,$1-$d-vc11.sln,$2-$d-vc11.vcxproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc12slns = \
$(foreach d,$(databases),$(call \
meta-vc12sln,$(src_root)/template-vc12.sln,$1-$d-vc12.sln,$2-$d-vc12.vcxproj,database,$d)$(literal_newline)\
$(literal_tab))@:

vc8slns = $(addprefix $1-,$(addsuffix -vc8.sln,$(databases)))
vc9slns = $(addprefix $1-,$(addsuffix -vc9.sln,$(databases)))
vc10slns = $(addprefix $1-,$(addsuffix -vc10.sln,$(databases)))
vc11slns = $(addprefix $1-,$(addsuffix -vc11.sln,$(databases)))
vc12slns = $(addprefix $1-,$(addsuffix -vc12.sln,$(databases)))

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
