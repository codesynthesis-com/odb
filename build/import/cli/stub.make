# file      : build/import/cli/stub.make
# copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
# license   : MIT; see accompanying LICENSE file

$(call include-once,$(scf_root)/import/cli/configuration-rules.make,$(dcf_root))

cli_installed :=

$(call -include,$(dcf_root)/import/cli/configuration-dynamic.make)

ifdef cli_installed

ifeq ($(cli_installed),y)

$(call export,cli: cli,cli-rules: $(scf_root)/import/cli/cli-cxx.make)

else

# Include export stub.
#
$(call include,$(scf_root)/export/cli/stub.make)

endif

else

.NOTPARALLEL:

endif
