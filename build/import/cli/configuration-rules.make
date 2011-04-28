# file      : build/import/cli/configuration-rules.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : MIT; see accompanying LICENSE file

$(dcf_root)/import/cli/configuration-dynamic.make: | $(dcf_root)/import/cli/.
	$(call message,,$(scf_root)/import/cli/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/cli/configuration-dynamic.make,\
rm -f $(dcf_root)/import/cli/configuration-dynamic.make)

endif
