# file      : buildfile
# license   : GNU GPL v2; see accompanying LICENSE file

./: {*/ -build/ -m4/} doc{INSTALL NEWS README} legal{GPLv2 LICENSE} manifest

# Don't install tests or the INSTALL file.
#
tests/:          install = false
doc{INSTALL}@./: install = false
