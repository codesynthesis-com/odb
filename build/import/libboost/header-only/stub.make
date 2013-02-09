# file      : build/import/libboost/header-only/stub.make
# copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libboost/configuration-rules.make,$(dcf_root))

libboost_installed :=

$(call -include,$(dcf_root)/import/libboost/configuration-dynamic.make)

ifdef libboost_installed

ifeq ($(libboost_installed),n)

$(call include-once,$(scf_root)/import/libboost/header-only/rules.make,$(dcf_root))

$(call export,\
  l: ,\
  cpp-options: $(dcf_root)/import/libboost/header-only/header-only.l.cpp-options)

endif

else

.NOTPARALLEL:

endif
