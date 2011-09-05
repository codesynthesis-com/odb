# file      : build/import/liboci/configuration-rules.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(dcf_root)/import/liboci/configuration-dynamic.make: | $(dcf_root)/import/liboci/.
	$(call message,,$(scf_root)/import/liboci/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/liboci/configuration-dynamic.make,\
rm -f $(dcf_root)/import/liboci/configuration-dynamic.make)

endif
