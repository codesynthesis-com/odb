# file      : build/import/libqt/core/stub.make
# copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libqt/configuration-rules.make,$(dcf_root))

libqt_version :=
libqt_installed :=

$(call -include,$(dcf_root)/import/libqt/configuration-dynamic.make)

ifdef libqt_installed

ifeq ($(libqt_installed),y)

$(call export,\
  l: $(libqt_core_libs),\
  cpp-options: ,\
  cpp-options-inline: $(libqt_core_cppflags))

else

$(call include-once,$(scf_root)/import/libqt/core/rules.make,$(dcf_root))

$(call export,\
  l: $(dcf_root)/import/libqt/core/core.l,\
  cpp-options: $(dcf_root)/import/libqt/core/core.l.cpp-options,\
  cpp-options-inline: )

endif

else

.NOTPARALLEL:

endif
