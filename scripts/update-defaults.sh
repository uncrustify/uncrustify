#! /bin/sh
#
./src/uncrustify -c /dev/null --update-config-with-doc > etc/defaults.cfg
cp etc/defaults.cfg documentation/htdocs/default.cfg
./src/uncrustify --show-config > documentation/htdocs/config.txt
./src/uncrustify --universalindent > etc/uigui_uncrustify.ini
