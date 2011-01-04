# file      : build/import/libodb-tracer/configuration-rules.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libodb-tracer/configuration-dynamic.make: | $(dcf_root)/import/libodb-tracer/.
	$(call message,,$(scf_root)/import/libodb-tracer/configure $@)

ifndef %foreign%

$(dcf_root)/.disfigure::
	$(call message,rm $(dcf_root)/import/libodb-tracer/configuration-dynamic.make,\
rm -f $(dcf_root)/import/libodb-tracer/configuration-dynamic.make)

endif
