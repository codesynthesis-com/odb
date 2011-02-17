# file      : build/import/libboost/date-time/rules.make
# author    : Boris Kolpackov <boris@kolpackov.net>
# copyright : Copyright (c) 2005-2011 Boris Kolpackov
# license   : GNU GPL v2; see accompanying LICENSE file

$(dcf_root)/import/libboost/%: root := $(libboost_root)

$(dcf_root)/import/libboost/date-time/date-time.l: \
  | $(dcf_root)/import/libboost/date-time/.

ifeq ($(libboost_type),archive)

ifeq ($(libboost_system),y)
$(dcf_root)/import/libboost/date-time/date-time.l: \
  $(libboost_root)/stage/lib/libboost_date_time$(libboost_suffix).a \
  $(libboost_root)/stage/lib/libboost_system$(libboost_suffix).a
else
$(dcf_root)/import/libboost/date-time/date-time.l: \
  $(libboost_root)/stage/lib/libboost_date_time$(libboost_suffix).a
endif
	@echo $^ >$@

else

ifeq ($(libboost_system),y)
$(dcf_root)/import/libboost/date-time/date-time.l: \
  $(libboost_root)/stage/lib/libboost_date_time$(libboost_suffix).so \
  $(libboost_root)/stage/lib/libboost_system$(libboost_suffix).so
else
$(dcf_root)/import/libboost/date-time/date-time.l: \
  $(libboost_root)/stage/lib/libboost_date_time$(libboost_suffix).so
endif
	@echo $^ >$@
	@echo rpath:$(root)/stage/lib >>$@

endif


$(dcf_root)/import/libboost/date-time/date-time.l.cpp-options: \
  | $(dcf_root)/import/libboost/date-time/.
	@echo include: -I$(root) >$@

ifndef %foreign%

disfigure::
	$(call message,rm $(dcf_root)/import/libboost/date-time/date-time.l,\
rm -f $(dcf_root)/import/libboost/date-time/date-time.l)
	$(call message,,\
rm -f $(dcf_root)/import/libboost/date-time/date-time.l.cpp-options)

endif
