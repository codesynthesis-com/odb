# file      : build/configuration.make
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

$(call include-once,$(scf_root)/configuration-rules.make,$(dcf_root))

# Dynamic configuration.
#
db_id :=

$(call -include,$(dcf_root)/configuration-dynamic.make)

ifdef db_id

$(out_root)/%: db_id := $(db_id)

else

.NOTPARALLEL:

endif
