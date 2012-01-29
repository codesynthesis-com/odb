# file      : build/configuration-rules.make
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/configuration-dynamic.make: | $(dcf_root)/.
	$(call message,,$(scf_root)/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $$1,rm -f $$1,$(dcf_root)/configuration-dynamic.make)
	$(call message,rm $$1,rm -f $$1,$(dcf_root)/db.options)
	$(call message,rm $$1,rm -f $$1,$(dcf_root)/db-driver)

endif

ifeq ($(.DEFAULT_GOAL),$(dcf_root)/configuration-dynamic.make)
.DEFAULT_GOAL :=
endif
