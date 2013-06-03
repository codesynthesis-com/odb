# file      : build/configuration-rules.make
# copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/configuration-dynamic.make: | $(dcf_root)/.
	$(call message,,$(scf_root)/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $$1,rm -f $$1,$(dcf_root)/configuration-dynamic.make)
	$(call message,rm $$1,rm -f $$1,$(dcf_root)/*.options)
	$(call message,rm $$1,rm -f $$1,$(dcf_root)/*-driver)

endif

ifeq ($(.DEFAULT_GOAL),$(dcf_root)/configuration-dynamic.make)
.DEFAULT_GOAL :=
endif
