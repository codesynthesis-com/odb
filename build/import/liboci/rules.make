# file      : build/import/liboci/rules.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

$(dcf_root)/import/liboci/%: root := $(liboci_root)
$(dcf_root)/import/liboci/oci.l: | $(dcf_root)/import/liboci/.

$(dcf_root)/import/liboci/oci.l: $(liboci_root)/lib/libclntsh.so
	@echo $< >$@
	@echo rpath:$(root)/lib >>$@

$(dcf_root)/import/liboci/oci.l.cpp-options: | $(dcf_root)/import/liboci/.
	@echo include: -I$(root)/rdbms/public >$@

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/liboci/oci.l,\
rm -f $(dcf_root)/import/liboci/oci.l)
	$(call message,rm $(dcf_root)/import/liboci/oci.l.cpp-options,\
rm -f $(dcf_root)/import/liboci/oci.l.cpp-options)

endif
