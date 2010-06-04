# file      : makefile
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))build/bootstrap.make

default  := $(out_base)/
test     := $(out_base)/.test
clean    := $(out_base)/.clean

# Build.
#
$(default):            \
$(out_base)/tracer/    \
$(out_base)/common/    \
$(out_base)/libcommon/

# Test.
#
$(test): $(out_base)/tracer/.test $(out_base)/common/.test

# Clean.
#
$(clean):                    \
$(out_base)/tracer/.clean    \
$(out_base)/common/.clean    \
$(out_base)/libcommon/.clean

$(call import,$(src_base)/tracer/makefile)
$(call import,$(src_base)/common/makefile)
$(call import,$(src_base)/libcommon/makefile)
