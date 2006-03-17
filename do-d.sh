#! /bin/sh

src/uncrustify -c etc/d.cfg -f $1 -p parsed.txt > out.d 2> out.err.txt
