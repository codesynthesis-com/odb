# file      : build/import/libmysqlclient_r/configuration-rules.make
# copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libmysqlclient_r/configuration-dynamic.make: | $(dcf_root)/import/libmysqlclient_r/.
	$(call message,,$(scf_root)/import/libmysqlclient_r/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libmysqlclient_r/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libmysqlclient_r/configuration-dynamic.make)

endif
