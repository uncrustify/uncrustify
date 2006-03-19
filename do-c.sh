#! /bin/sh

src/uncrustify -c etc/ben.cfg -f $1 -p parsed.txt > out.c 2> out.err.txt
