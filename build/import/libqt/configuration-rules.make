# file      : build/import/libqt/configuration-rules.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libqt/configuration-dynamic.make: | $(dcf_root)/import/libqt/.
	$(call message,,$(scf_root)/import/libqt/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libqt/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libqt/configuration-dynamic.make)

endif
