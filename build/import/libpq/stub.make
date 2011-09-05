# file      : build/import/libpq/stub.make
# author    : Constantin Michael <constantin@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/libpq/configuration-rules.make,$(dcf_root))

libpq_installed :=

$(call -include,$(dcf_root)/import/libpq/configuration-dynamic.make)

ifdef libpq_installed

ifeq ($(libpq_installed),y)

$(call export,\
  l: $(libpq_libs),\
  cpp-options: ,\
  cpp-options-inline: $(libpq_cppflags))

else

$(call include-once,$(scf_root)/import/libpq/rules.make,$(dcf_root))

$(call export,\
  l: $(dcf_root)/import/libpq/pq.l,\
  cpp-options: $(dcf_root)/import/libpq/pq.l.cpp-options)

endif

else

.NOTPARALLEL:

endif
