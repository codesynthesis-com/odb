# file      : build/import/odb/hxx-cxx.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v3; see accompanying LICENSE file

# Here we are operating in the importing project's space, not in odb's.
#

# Get the C++ configuration (file extensions, and extra CPP options).
#
$(call include,$(bld_root)/cxx/configuration.make)

odb_pattern :=                    \
$(out_base)/%-odb.$(cxx_s_suffix) \
$(out_base)/%-odb.$(cxx_h_suffix) \
$(out_base)/%-odb.$(cxx_i_suffix) \
$(out_base)/%.sql

$(odb_pattern): odb_options := \
--hxx-suffix .$(cxx_h_suffix)  \
--ixx-suffix .$(cxx_i_suffix)  \
--cxx-suffix .$(cxx_s_suffix)

$(odb_pattern): odb-expand-cpp-options-impl = \
$(if $1,$(shell sed -e 's%include: \(.*\)%\1%' -e t -e d $1))

$(odb_pattern): odb-expand-cpp-options = \
$(call odb-expand-cpp-options-impl,$(filter %.cpp-options,$1))

.PRECIOUS: $(odb_pattern)

ifeq ($(out_base),$(src_base))

$(odb_pattern): $(src_base)/%.$(cxx_h_suffix)
	$(call message,odb $<,$(odb) $(cpp_options) \
$(call expand-cpp-options,$^) $(cxx_pp_extra_options) $(odb_options) \
--output-dir $(dir $@) $<)

else

$(odb_pattern): $(src_base)/%.$(cxx_h_suffix) | $$(dir $$@).
	$(call message,odb $<,$(odb) $(cpp_options) \
$(call expand-cpp-options,$^) $(cxx_pp_extra_options) $(odb_options) \
--output-dir $(dir $@) $<)

$(odb_pattern): $(out_base)/%.$(cxx_h_suffix) | $$(dir $$@).
	$(call message,odb $<,$(odb) $(cpp_options) \
$(call expand-cpp-options,$^) $(cxx_pp_extra_options) $(odb_options) \
--output-dir $(dir $@) $<)
endif

.PHONY: $(out_base)/%-odb.cxx.hxx.clean

$(out_base)/%-odb.cxx.hxx.clean: cxx_s_suffix := $(cxx_s_suffix)
$(out_base)/%-odb.cxx.hxx.clean: cxx_h_suffix := $(cxx_h_suffix)
$(out_base)/%-odb.cxx.hxx.clean: cxx_i_suffix := $(cxx_i_suffix)

$(out_base)/%-odb.cxx.hxx.clean:
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.hxx.clean=.$(cxx_s_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.hxx.clean=.$(cxx_h_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.hxx.clean=.$(cxx_i_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:-odb.cxx.hxx.clean=.sql))
