# file      : build/import/libboost/date-time/stub.make
# copyright : Copyright (c) 2005-2011 Boris Kolpackov
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libboost/configuration-rules.make,$(dcf_root))

libboost_installed :=

$(call -include,$(dcf_root)/import/libboost/configuration-dynamic.make)

ifdef libboost_installed

ifeq ($(libboost_installed),y)

ifeq ($(libboost_system),y)
$(call export,l: -lboost_date_time$(libboost_suffix) -lboost_system$(libboost_suffix),cpp_options: )
else
$(call export,l: -lboost_date_time$(libboost_suffix),cpp_options: )
endif

else

$(call include-once,$(scf_root)/import/libboost/date-time/rules.make,$(dcf_root))

$(call export,\
  l: $(dcf_root)/import/libboost/date-time/date-time.l,\
  cpp-options: $(dcf_root)/import/libboost/date-time/date-time.l.cpp-options)

endif

else

.NOTPARALLEL:

endif
