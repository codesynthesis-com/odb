# file      : buildfile
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : ODB NCUEL; see accompanying LICENSE file

./: {*/ -build/ -m4/} doc{NCUEL INSTALL LICENSE NEWS README} manifest

# Don't install tests or the INSTALL file.
#
tests/:          install = false
doc{INSTALL}@./: install = false
