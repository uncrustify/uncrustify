#!/bin/bash
#
# 27. 10. 2016
#
SRC="./src"
#
where=`pwd`
#
# build the lists
cd ${SRC}
list_of_H=`ls *.h`
list_of_C=`ls *.cpp`
cd ${where}
#
mkdir -p results
#
for file in ${list_of_H} ${list_of_C}
do
  uncrustify -q -c ./forUncrustifySources.cfg -f ${SRC}/${file} -o results/${file}
  diff ${SRC}/${file} results/${file} > /dev/null 2>&1
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with "${file}
    echo "the command is: diff ${SRC}/${file} results/${file}"
  else
    rm results/${file}
  fi
done
rmdir results
