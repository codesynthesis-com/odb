# file      : build/import/libboost/configuration-rules.make
# copyright : Copyright (c) 2005-2011 Boris Kolpackov
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libboost/configuration-dynamic.make: | $(dcf_root)/import/libboost/.
	$(call message,,$(scf_root)/import/libboost/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libboost/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libboost/configuration-dynamic.make)

endif
