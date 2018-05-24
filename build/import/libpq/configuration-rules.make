# file      : build/import/libpq/configuration-rules.make
# copyright : Copyright (c) 2005-2018 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libpq/configuration-dynamic.make: | $(dcf_root)/import/libpq/.
	$(call message,,$(scf_root)/import/libpq/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libpq/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libpq/configuration-dynamic.make)

endif
