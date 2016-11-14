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
  .build/uncrustify -q -c ./forUncrustifySources.cfg -f ${SRC}/${file} -o results/${file}
  cmp -s ${SRC}/${file} results/${file}
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with "${file}
    echo "use: diff ${SRC}/${file} results/${file} to find why"
  else
    rm results/${file}
  fi
done
rmdir --ignore-fail-on-non-empty results
if [[ -d results ]]
then
  echo "some problem(s) are still present"
  exit 1
else
  echo "all sources are uncrustify-ed"
  exit 0
fi
