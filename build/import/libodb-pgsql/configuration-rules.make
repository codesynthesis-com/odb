# file      : build/import/libodb-pgsql/configuration-rules.make
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodb-pgsql/configuration-dynamic.make: | $(dcf_root)/import/libodb-pgsql/.
	$(call message,,$(scf_root)/import/libodb-pgsql/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-pgsql/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-pgsql/configuration-dynamic.make)

endif
