# file      : build/import/libodb-qt/configuration-rules.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodb-qt/configuration-dynamic.make: | $(dcf_root)/import/libodb-qt/.
	$(call message,,$(scf_root)/import/libodb-qt/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-qt/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-qt/configuration-dynamic.make)

endif
