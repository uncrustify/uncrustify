#!/bin/bash
#
# 22 februar 2017
#
SRC="./src"
#
where=`pwd`
#
# build the lists
cd ${SRC}
list_of_C=`ls *.cpp`
list_of_H=`ls *.h`
cd ${where}
#
RESULTS="./results"
#
rm -rf ${RESULTS}
mkdir ${RESULTS}
#
cp build/compile_commands.json ${SRC}
#
list_of_Check="readability-else-after-return"
#
for file in ${list_of_H} ${list_of_C}
do
  echo "test for "${file}
  OUTPUT=${RESULTS}/${file}.txt
  for check in ${list_of_Check}
  do
    clang-tidy -checks="-*, ${check}" -header-filter="./src/*" ${SRC}/${file} \
      > ${OUTPUT} 2>/dev/null
    #ls -l ${OUTPUT}
    if [[ -s ${OUTPUT} ]]
    then
      #echo "nicht zero"
      head ${OUTPUT}
    else
      #echo "zero"
      rm -f ${OUTPUT}
    fi
  done
done
#
rmdir --ignore-fail-on-non-empty ${RESULTS}
if [[ -d ${RESULTS} ]]
then
  echo "some problem(s) are still present"
  exit 1
else
  echo "all clang-tidy are OK"
  exit 0
fi
