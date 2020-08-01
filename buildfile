# file      : buildfile
# license   : ODB NCUEL; see accompanying LICENSE file

./: {*/ -build/ -m4/} doc{INSTALL NEWS README} legal{NCUEL LICENSE} manifest

# Don't install tests or the INSTALL file.
#
tests/:          install = false
doc{INSTALL}@./: install = false
