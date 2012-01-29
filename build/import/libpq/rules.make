# file      : build/import/libpq/rules.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libpq/%: root := $(libpq_root)

ifeq ($(libpq_type),archive)

$(dcf_root)/import/libpq/pq.l: $(libpq_root)/libpq/.libs/libpq.a
	@echo $< >$@
else

$(dcf_root)/import/libpq/pq.l: $(libpq_root)/libpq/.libs/libpq.so
	@echo $< >$@
	@echo rpath:$(root)/libpq/.libs >>$@
endif

$(dcf_root)/import/libpq/pq.l.cpp-options:
	@ln -s $(root)/include $(dcf_root)/import/libpq/pq
	@echo include: -I$(dcf_root)/import/libpq >$@

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/pq/pq.l,\
rm -f $(dcf_root)/import/pq/pq.l)
	$(call message,,rm -f $(dcf_root)/import/libpq/pq)
	$(call message,,rm -f $(dcf_root)/import/libpq/pq.l.cpp-options)

endif
