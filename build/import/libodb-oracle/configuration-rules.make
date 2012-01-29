# file      : build/import/libodb-oracle/configuration-rules.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(dcf_root)/import/libodb-oracle/configuration-dynamic.make: | $(dcf_root)/import/libodb-oracle/.
	$(call message,,$(scf_root)/import/libodb-oracle/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-oracle/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-oracle/configuration-dynamic.make)

endif
