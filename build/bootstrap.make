# file      : build/bootstrap.make
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

project_name := libodb-mssql

# First try to include the bundled bootstrap.make if it exist. If that
# fails, let make search for the external bootstrap.make.
#
build := build-0.3

-include $(dir $(lastword $(MAKEFILE_LIST)))../../$(build)/bootstrap.make

ifeq ($(patsubst %build/bootstrap.make,,$(lastword $(MAKEFILE_LIST))),)
include $(build)/bootstrap.make
endif


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

ifneq ($(filter $(.DEFAULT_GOAL),test dist clean),)
.DEFAULT_GOAL :=
endif

endif

# Make sure the distribution prefix is set if the goal is dist.
#
ifneq ($(filter $(MAKECMDGOALS),dist),)
ifeq ($(dist_prefix),)
$(error dist_prefix is not set)
endif
endif

# If we don't have dependency auto-generation then we need to manually
# make sure that generated files are generated before C++ file are
# compiler. To do this we make the object files ($2) depend in order-
# only on generated files ($3).
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
