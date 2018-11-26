#! /bin/sh
#
./build/uncrustify -c /dev/null --update-config-with-doc > etc/defaults.cfg
cp etc/defaults.cfg documentation/htdocs/default.cfg
./build/uncrustify --show-config > documentation/htdocs/config.txt
./build/uncrustify --universalindent > etc/uigui_uncrustify.ini
