# file      : build/import/libpq/configuration-rules.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libpq/configuration-dynamic.make: | $(dcf_root)/import/libpq/.
	$(call message,,$(scf_root)/import/libpq/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libpq/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libpq/configuration-dynamic.make)

endif
