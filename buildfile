# file      : buildfile
# copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

d = odb/pgsql/ tests/
./: $d doc{GPLv2 INSTALL LICENSE NEWS README version} file{manifest}
include $d

doc{INSTALL}@./: install = false
