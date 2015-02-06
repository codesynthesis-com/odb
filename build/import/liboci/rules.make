# file      : build/import/liboci/rules.make
# copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/liboci/%: root := $(liboci_root)
$(dcf_root)/import/liboci/%: include := $(liboci_include)
$(dcf_root)/import/liboci/%: lib := $(liboci_lib)
$(dcf_root)/import/liboci/oci.l: | $(dcf_root)/import/liboci/.

$(dcf_root)/import/liboci/oci.l: $(liboci_lib)
	@echo $(lib) >$@
	@echo rpath:$(dir $(lib)) >>$@

$(dcf_root)/import/liboci/oci.l.cpp-options: | $(dcf_root)/import/liboci/.
	@echo include: -I$(include) >$@

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/liboci/oci.l,\
rm -f $(dcf_root)/import/liboci/oci.l)
	$(call message,rm $(dcf_root)/import/liboci/oci.l.cpp-options,\
rm -f $(dcf_root)/import/liboci/oci.l.cpp-options)

endif
