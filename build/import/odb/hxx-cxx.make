# file      : build/import/odb/hxx-cxx.make
# license   : GNU GPL v3; see accompanying LICENSE file

# Here we are operating in the importing project's space, not in odb's.
#

# Get the C++ configuration (file extensions, and extra CPP options).
#
$(call include,$(bld_root)/cxx/configuration.make)

odb_databases := mysql sqlite pgsql oracle mssql

odb_pattern :=                    \
$(out_base)/%-odb.$(cxx_s_suffix) \
$(out_base)/%-odb.$(cxx_h_suffix) \
$(out_base)/%-odb.$(cxx_i_suffix) \
$(out_base)/%.sql

odb_patterns := $(odb_pattern)

define odb-db-pattern
odb_$1_pattern :=                         \
$$(out_base)/%-odb-$1.$$(cxx_s_suffix) \
$$(out_base)/%-odb-$1.$$(cxx_h_suffix) \
$$(out_base)/%-odb-$1.$$(cxx_i_suffix) \
$$(out_base)/%-$1.sql

odb_patterns += $$(odb_$1_pattern)

endef # Trailing newline is important.

$(foreach d,$(odb_databases),$(eval $(call odb-db-pattern,$d)))

$(odb_patterns): odb_options := \
--hxx-suffix .$(cxx_h_suffix)   \
--ixx-suffix .$(cxx_i_suffix)   \
--cxx-suffix .$(cxx_s_suffix)

$(odb_patterns): odb-expand-cpp-options-impl = \
$(if $1,$(shell sed -e 's%include: \(.*\)%\1%' -e t -e d $1))

$(odb_patterns): odb-expand-cpp-options = \
$(call odb-expand-cpp-options-impl,$(filter %.cpp-options,$1))

# We only check for the long option name to avoid false positives.
#
$(odb_pattern): odb-default-database = \
$(if $(filter --multi-database ,$(odb_options)),--database common )

$(odb_pattern): odb-default-database-message = \
$(if $(filter --multi-database ,$(odb_options)),[common] )

.PRECIOUS: $(odb_patterns)

ifeq ($(out_base),$(src_base))

$(odb_pattern): $(src_base)/%.$(cxx_h_suffix)
	$(call message,odb $(call odb-default-database-message)$<,$(odb) \
$(cpp_options) $(call odb-expand-cpp-options,$^) $(cxx_pp_extra_options) \
$(odb_options) $(call odb-default-database)--output-dir $(dir $@) $<)

define odb-db-rule
$$(odb_$1_pattern): $$(src_base)/%.$$(cxx_h_suffix)
	$$(call message,odb [$1] $$<,$$(odb) $$(cpp_options) \
$$(call odb-expand-cpp-options,$$^) $$(cxx_pp_extra_options) $$(odb_options) \
--database $1 --output-dir $$(dir $$@) $$<)

endef # Trailing newline is important.

else

$(odb_pattern): $(src_base)/%.$(cxx_h_suffix) | $$(dir $$@).
	$(call message,odb $(call odb-default-database-message)$<,$(odb) \
$(cpp_options) $(call odb-expand-cpp-options,$^) $(cxx_pp_extra_options) \
$(odb_options) $(call odb-default-database)--output-dir $(dir $@) $<)

$(odb_pattern): $(out_base)/%.$(cxx_h_suffix) | $$(dir $$@).
	$(call message,odb $(call odb-default-database-message)$<,$(odb) \
$(cpp_options) $(call odb-expand-cpp-options,$^) $(cxx_pp_extra_options) \
$(odb_options) $(call odb-default-database) --output-dir $(dir $@) $<)

define odb-db-rule
$$(odb_$1_pattern): $$(src_base)/%.$$(cxx_h_suffix) | $$$$(dir $$$$@).
	$$(call message,odb [$1] $$<,$$(odb) $$(cpp_options) \
$$(call odb-expand-cpp-options,$$^) $$(cxx_pp_extra_options) $$(odb_options) \
--database $1 --output-dir $$(dir $$@) $$<)

$$(odb_$1_pattern): $$(out_base)/%.$$(cxx_h_suffix) | $$$$(dir $$$$@).
	$$(call message,odb [$1] $$<,$$(odb) $$(cpp_options) \
$$(call odb-expand-cpp-options,$$^) $$(cxx_pp_extra_options) $$(odb_options) \
--database $1 --output-dir $$(dir $$@) $$<)

endef # Trailing newline is important.
endif

$(foreach d,$(odb_databases),$(eval $(call odb-db-rule,$d)))

# Clean.
#
.PHONY: $(out_base)/%-odb.cxx.hxx.clean

$(out_base)/%-odb.cxx.hxx.clean: cxx_s_suffix := $(cxx_s_suffix)
$(out_base)/%-odb.cxx.hxx.clean: cxx_h_suffix := $(cxx_h_suffix)
$(out_base)/%-odb.cxx.hxx.clean: cxx_i_suffix := $(cxx_i_suffix)

$(out_base)/%-odb.cxx.hxx.clean:
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.hxx.clean=.$(cxx_s_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.hxx.clean=.$(cxx_h_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:.cxx.hxx.clean=.$(cxx_i_suffix)))
	$(call message,rm $$1,rm -f $$1,$(@:-odb.cxx.hxx.clean=.sql))

define odb-db-clean-rule
.PHONY: $$(out_base)/%-odb-$1.cxx.hxx.clean

$$(out_base)/%-odb-$1.cxx.hxx.clean: cxx_s_suffix := $$(cxx_s_suffix)
$$(out_base)/%-odb-$1.cxx.hxx.clean: cxx_h_suffix := $$(cxx_h_suffix)
$$(out_base)/%-odb-$1.cxx.hxx.clean: cxx_i_suffix := $$(cxx_i_suffix)

$$(out_base)/%-odb-$1.cxx.hxx.clean:
	$$(call message,rm $$$$1,rm -f $$$$1,$$(@:.cxx.hxx.clean=.$$(cxx_s_suffix)))
	$$(call message,rm $$$$1,rm -f $$$$1,$$(@:.cxx.hxx.clean=.$$(cxx_h_suffix)))
	$$(call message,rm $$$$1,rm -f $$$$1,$$(@:.cxx.hxx.clean=.$$(cxx_i_suffix)))
	$$(call message,rm $$$$1,rm -f $$$$1,$$(@:-odb-$1.cxx.hxx.clean=-$1.sql))

endef # Trailing newline is important.

$(foreach d,$(odb_databases),$(eval $(call odb-db-clean-rule,$d)))
