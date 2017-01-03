# file      : build/import/libqt/core/rules.make
# copyright : Copyright (c) 2005-2017 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libqt/%: root := $(libqt_root)
$(dcf_root)/import/libqt/core/core.l: | $(dcf_root)/import/libqt/core/.

ifeq ($(libqt_type),archive)
$(dcf_root)/import/libqt/core/core.l: $(libqt_root)/lib/libQtCore.a
	@echo $^ >$@
else
$(dcf_root)/import/libqt/core/core.l: $(libqt_root)/lib/libQtCore.so
	@echo $^ >$@
	@echo rpath:$(root)/lib >>$@
endif

$(dcf_root)/import/libqt/core/core.l.cpp-options: | $(dcf_root)/import/libqt/core/.
	@echo include: -I$(root)/include -I$(root)/include/QtCore >$@

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libqt/core/core.l,\
rm -f $(dcf_root)/import/libqt/core/core.l)
	$(call message,,rm -f $(dcf_root)/import/libqt/core/core.l.cpp-options)

endif
