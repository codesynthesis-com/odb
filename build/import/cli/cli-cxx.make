# file      : build/import/cli/cli-cxx.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : MIT; see accompanying LICENSE file

# Here we are operating in the importing project's space, not in
# cli's.
#

# Get the C++ file extensions.
#
$(call include,$(bld_root)/cxx/configuration-static.make)

cli_pattern :=                \
$(out_base)/%.$(cxx_s_suffix) \
$(out_base)/%.$(cxx_h_suffix) \
$(out_base)/%.$(cxx_i_suffix)

$(cli_pattern): cli_options := \
--hxx-suffix .$(cxx_h_suffix)  \
--ixx-suffix .$(cxx_i_suffix)  \
--cxx-suffix .$(cxx_s_suffix)

.PRECIOUS: $(cli_pattern)

ifeq ($(out_base),$(src_base))

$(cli_pattern): $(src_base)/%.cli
	$(call message,cli $<,$(cli) $(cli_options) --output-dir $(dir $@) $<)

else

$(cli_pattern): $(src_base)/%.cli | $$(dir $$@).
	$(call message,cli $<,$(cli) $(cli_options) --output-dir $(dir $@) $<)

$(cli_pattern): $(out_base)/%.cli | $$(dir $$@).
	$(call message,cli $<,$(cli) $(cli_options) --output-dir $(dir $@) $<)
endif

.PHONY: $(out_base)/%.cxx.cli.clean

$(out_base)/%.cxx.cli.clean: cxx_s_suffix := $(cxx_s_suffix)
$(out_base)/%.cxx.cli.clean: cxx_h_suffix := $(cxx_h_suffix)
$(out_base)/%.cxx.cli.clean: cxx_i_suffix := $(cxx_i_suffix)

$(out_base)/%.cxx.cli.clean:
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.cli.clean=.$(cxx_s_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.cli.clean=.$(cxx_h_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.cli.clean=.$(cxx_i_suffix)))
