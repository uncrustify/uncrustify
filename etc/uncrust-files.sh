#! /bin/sh

if [ -z "$1" ]; then
   echo "specify the file that contains a list of files"
   exit
fi

files=$(cat $1)

mkdir -p out

for item in $files ; do

  dn=$(dirname $item)
  mkdir -p out/$dn
  ~/bin/uncrustify -f $item -c ~/.uncrustify/xsupplicant.cfg > out/$item

done

