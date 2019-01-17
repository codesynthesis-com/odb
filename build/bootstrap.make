# file      : build/bootstrap.make
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
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

# Databases.
#
databases := mysql sqlite pgsql oracle mssql
$(dist): databases := $(databases)

# Aliases
#
default := $(out_base)/
driver  := $(out_base)/driver
test    := $(out_base)/.test
dist    := $(out_base)/.dist
clean   := $(out_base)/.clean

.PHONY: $(default) \
        $(test)    \
        $(dist)    \
        $(clean)

ifeq ($(db_id),common)
define db-test-target
test_$1 := $$(out_base)/.test-$1
.PHONY: $$(test_$1)
$$(test): $$(test_$1)

endef
$(foreach d,$(databases),$(eval $(call db-test-target,$d)))

define db-test-dir
$$(test_$1): $$(addprefix $$(out_base)/,$$(addsuffix /.test-$1,$2))

endef
endif

ifdef %interactive%
.PHONY: test dist clean
test:  $(test)
dist:  $(dist)
clean: $(clean)

ifeq ($(db_id),common)
define db-test-alias
.PHONY: test-$1
test-$1: $$(test_$1)

endef
$(foreach d,$(databases),$(eval $(call db-test-alias,$d)))
endif
endif

# Return the list of ODB-generated files given a list of header files.
#
ifdef db_id
ifneq ($(db_id),common)
odb-gen = $(foreach f,$(1:.hxx=),$(addprefix $f,-odb.hxx -odb.ixx -odb.cxx .sql))
else
odb-gen = $(foreach f,$(1:.hxx=),$(addprefix $f, -odb.hxx -odb.ixx -odb.cxx \
$(foreach d,$(databases),-odb-$d.hxx -odb-$d.ixx -odb-$d.cxx -$d.sql)))
endif
endif

# By default the ODB header is called test.hxx.
#
$(out_base)/.dist: export odb_header_stem := test

# Database schema creation.
#
ifneq ($(db_id),sqlite)
$(out_base)/.test $(addprefix $(out_base)/.test-,$(databases)): schema-body = \
$(call message,sql $$1,$(dcf_root)/$1-driver $$1,$2)$(literal_newline)$(literal_tab)

ifneq ($(db_id),common)
$(out_base)/.test: schema = \
$(foreach s,$(if $1,$1,$(out_base)/test.sql),$(call schema-body,$(db_id),$s))@:
else
define db-schema
$$(out_base)/.test-$1: schema-$1 = \
$$(foreach s,$$(if $$1,$$(filter %-$1.sql,$$1),$$(out_base)/test-$1.sql),$$(call schema-body,$1,$$s))@:

endef
$(foreach d,$(filter-out sqlite,$(databases)),$(eval $(call db-schema,$d)))
endif
endif

# Test rule templates.
#

# $1 database name in the multi-database mode and empty otherwise
# $2 optional test.std file suffix (e.g., '-mysql' for test-mysql.std)
#
define test-schemaless-rule
$$(test$(if $1,_$1)): $$(driver) $$(src_base)/test$2.std
	$$(call message,test$(if $1, [$1]) $$<,$$< $1 --options-file \
$$(dcf_root)/$(if $1,$1,$(db_id)).options >$$(out_base)/test.out)
	$$(call message,,diff -u $$(src_base)/test$2.std $$(out_base)/test.out)
	$$(call message,,rm -f $$(out_base)/test.out)

endef

# $1 database name in the multi-database mode and empty otherwise
# $2 optional list of schema files, by default test.sql
# $3 optional test.std file suffix (e.g., '-mysql' for test-mysql.std)
#
define test-rule
$$(test$(if $1,_$1)): $$(driver) $$(src_base)/test$3.std
	$$(call schema$(if $1,-$1),$2)
	$$(call message,test$(if $1, [$1]) $$<,$$< $1 --options-file \
$$(dcf_root)/$(if $1,$1,$(db_id)).options >$$(out_base)/test.out)
	$$(call message,,diff -u $$(src_base)/test$3.std $$(out_base)/test.out)
	$$(call message,,rm -f $$(out_base)/test.out)

endef

# Dist setup.
#
ifneq ($(filter $(MAKECMDGOALS),dist),)

# Make sure the distribution prefix is set if the goal is dist.
#
ifeq ($(dist_prefix),)
$(error dist_prefix is not set)
endif

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
$(dist): meta-vc8sln1 = \
$(call meta-vc8sln,$(src_root)/template-vc8.sln,$1-vc8.sln,$2-vc8.vcproj)

$(dist): meta-vc8slns = \
$(foreach d,$(databases),$(call \
meta-vc8sln,$(src_root)/template-vc8.sln,$1-$d-vc8.sln,$2-$d-vc8.vcproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc9sln1 = \
$(call meta-vc9sln,$(src_root)/template-vc9.sln,$1-vc9.sln,$2-vc9.vcproj)

$(dist): meta-vc9slns = \
$(foreach d,$(databases),$(call \
meta-vc9sln,$(src_root)/template-vc9.sln,$1-$d-vc9.sln,$2-$d-vc9.vcproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc10sln1 = \
$(call meta-vc10sln,$(src_root)/template-vc10.sln,$1-vc10.sln,$2-vc10.vcxproj)

$(dist): meta-vc10slns = \
$(foreach d,$(databases),$(call \
meta-vc10sln,$(src_root)/template-vc10.sln,$1-$d-vc10.sln,$2-$d-vc10.vcxproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc11sln1 = \
$(call meta-vc11sln,$(src_root)/template-vc11.sln,$1-vc11.sln,$2-vc11.vcxproj)

$(dist): meta-vc11slns = \
$(foreach d,$(databases),$(call \
meta-vc11sln,$(src_root)/template-vc11.sln,$1-$d-vc11.sln,$2-$d-vc11.vcxproj,database,$d)$(literal_newline)\
$(literal_tab))@:

$(dist): meta-vc12sln1 = \
$(call meta-vc12sln,$(src_root)/template-vc12.sln,$1-vc12.sln,$2-vc12.vcxproj)

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
# Also make generated files depend on config.h so that we don't get
# annoying noise during dependency auto-generation.
#
ifeq ($(cxx_id),generic)

define include-dep
$(if $2,$(eval $2: | $3)) \
$(if $(and $3,$(subst $(out_root)/libcommon/common/config.h,,$3)),$(eval \
$3: $(out_root)/libcommon/common/config.h))
endef

else

define include-dep
$(if $(and $3,$(subst $(out_root)/libcommon/common/config.h,,$3)),$(eval \
$3: $(out_root)/libcommon/common/config.h)) \
$(call -include,$1)
endef

endif

# Don't include dependency info for certain targets.
#
ifneq ($(filter $(MAKECMDGOALS),clean disfigure dist),)
include-dep =
endif

.DEFAULT_GOAL := $(def_goal)
