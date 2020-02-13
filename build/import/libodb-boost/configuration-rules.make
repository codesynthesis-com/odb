# file      : build/import/libodb-boost/configuration-rules.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodb-boost/configuration-dynamic.make: | $(dcf_root)/import/libodb-boost/.
	$(call message,,$(scf_root)/import/libodb-boost/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-boost/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-boost/configuration-dynamic.make)

endif
