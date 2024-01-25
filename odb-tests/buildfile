# file      : buildfile
# license   : GNU GPL v2; see accompanying LICENSE file

./: libcommon/ common/ doc{README.md} legal{GPLv2 LICENSE} manifest

./: mysql/:  include = ($mysql  && !$multi)
./: sqlite/: include = ($sqlite && !$multi)
./: pgsql/:  include = ($pgsql  && !$multi)

./: testscript{*}: include = adhoc
