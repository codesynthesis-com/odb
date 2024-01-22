# file      : buildfile
# license   : GNU GPL v3; see accompanying LICENSE file

./: {*/ -build/ -m4/} doc{INSTALL NEWS README} legal{GPLv3 LICENSE} manifest

# Don't install tests or the INSTALL file.
#
tests/:          install = false
doc{INSTALL}@./: install = false
