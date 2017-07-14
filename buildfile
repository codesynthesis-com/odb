# file      : buildfile
# copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

./: {*/ -build/ -m4/} doc{GPLv2 INSTALL LICENSE NEWS README version} \
    file{manifest}

# Don't install tests or the INSTALL file.
#
dir{tests/}:     install = false
doc{INSTALL}@./: install = false
