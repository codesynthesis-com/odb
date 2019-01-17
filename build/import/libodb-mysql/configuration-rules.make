# file      : build/import/libodb-mysql/configuration-rules.make
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodb-mysql/configuration-dynamic.make: | $(dcf_root)/import/libodb-mysql/.
	$(call message,,$(scf_root)/import/libodb-mysql/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-mysql/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-mysql/configuration-dynamic.make)

endif
