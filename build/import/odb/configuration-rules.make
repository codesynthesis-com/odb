# file      : build/import/odb/configuration-rules.make
# copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
# license   : GNU GPL v3; see accompanying LICENSE file

$(dcf_root)/import/odb/configuration-dynamic.make: | $(dcf_root)/import/odb/.
	$(call message,,$(scf_root)/import/odb/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/odb/configuration-dynamic.make,\
rm -f $(dcf_root)/import/odb/configuration-dynamic.make)

endif
