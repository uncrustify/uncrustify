#! /bin/sh
src/uncrustify --update-config-with-doc -c /dev/null -o etc/defaults.cfg
src/uncrustify --show-config > documentation/htdocs/config.txt

