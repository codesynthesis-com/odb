# file      : build/import/libodb-oracle/configuration-rules.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodb-oracle/configuration-dynamic.make: | $(dcf_root)/import/libodb-oracle/.
	$(call message,,$(scf_root)/import/libodb-oracle/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-oracle/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-oracle/configuration-dynamic.make)

endif
