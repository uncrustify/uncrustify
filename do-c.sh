#! /bin/sh

src/uncrustify -c etc/ben.cfg -p parsed.txt -f $1 $2 $3 $4  > out.c 2> out.err.txt 
