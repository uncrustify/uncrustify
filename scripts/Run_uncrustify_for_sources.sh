#!/bin/bash
#
# 18 12. 2016
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
RESULTS="./results"
#
rm -rf ${RESULTS}
mkdir ${RESULTS}
#
#find . -name uncrustify
#ls -l ./build/uncrustify
for file in ${list_of_H} ${list_of_C}
do
  ./build/uncrustify -q -c ./forUncrustifySources.cfg -f ${SRC}/${file} -o ${RESULTS}/${file}
  cmp -s ${SRC}/${file} ${RESULTS}/${file}
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with "${file}
    echo "use: diff ${SRC}/${file} ${RESULTS}/${file} to find why"
    diff ${SRC}/${file} ${RESULTS}/${file}
  else
    rm ${RESULTS}/${file}
  fi
done
case $( uname -s ) in
Darwin) rmdir ${RESULTS};;
*)      rmdir --ignore-fail-on-non-empty ${RESULTS};;
esac
if [[ -d ${RESULTS} ]]
then
  echo "some problem(s) are still present"
  exit 1
else
  echo "all sources are uncrustify-ed"
  exit 0
fi
