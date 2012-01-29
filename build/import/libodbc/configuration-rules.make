# file      : build/import/libodbc/configuration-rules.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodbc/configuration-dynamic.make: | $(dcf_root)/import/libodbc/.
	$(call message,,$(scf_root)/import/libodbc/configure $@)

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libodbc/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodbc/configuration-dynamic.make)

endif
