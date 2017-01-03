# file      : buildfile
# copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

d = odb/pgsql/ tests/
./: $d doc{GPLv2 INSTALL LICENSE NEWS README version} file{manifest}
include $d

# Don't install tests or the INSTALL file.
#
dir{tests/}: install = false
doc{INSTALL}@./: install = false
