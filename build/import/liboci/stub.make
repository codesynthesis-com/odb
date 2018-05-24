# file      : build/import/liboci/stub.make
# copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/liboci/configuration-rules.make,$(dcf_root))

liboci_root :=

$(call -include,$(dcf_root)/import/liboci/configuration-dynamic.make)

ifdef liboci_root

$(call include-once,$(scf_root)/import/liboci/rules.make,$(dcf_root))

$(call export,\
  l: $(dcf_root)/import/liboci/oci.l,\
  cpp-options: $(dcf_root)/import/liboci/oci.l.cpp-options)

else

.NOTPARALLEL:

endif
