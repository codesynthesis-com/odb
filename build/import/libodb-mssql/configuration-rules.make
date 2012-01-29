# file      : build/import/libodb-mssql/configuration-rules.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(dcf_root)/import/libodb-mssql/configuration-dynamic.make: | $(dcf_root)/import/libodb-mssql/.
	$(call message,,$(scf_root)/import/libodb-mssql/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-mssql/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-mssql/configuration-dynamic.make)

endif
