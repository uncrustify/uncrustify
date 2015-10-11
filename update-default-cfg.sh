#! /bin/sh
build/src/uncrustify --update-config-with-doc -c /dev/null -o etc/defaults.cfg
cp etc/defaults.cfg documentation/htdocs/default.cfg
build/src/uncrustify --show-config > documentation/htdocs/config.txt
