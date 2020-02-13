# file      : build/import/libboost/header-only/rules.make
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libboost/%: root := $(libboost_root)

$(dcf_root)/import/libboost/header-only/header-only.l.cpp-options: \
  | $(dcf_root)/import/libboost/header-only/.
	@echo include: -I$(root) >$@

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libboost/header-only/header-only.l,\
rm -f $(dcf_root)/import/libboost/header-only/header-only.l.cpp-options)

endif
