#!/bin/bash
#
# 2017-02-27
#
script_dir="$(dirname "$(readlink -f "$0")")"
#
SRC="${script_dir}/../src"
BUILD="${script_dir}/../build"
#
where=`pwd`
#
# build the lists
cd ${SRC}
list_of_C=`ls *.cpp`
list_of_H=`ls *.h`
list_of_files="${list_of_C} ${list_of_H}" 
cd ${where}
#
RESULTS="${script_dir}/../results"
#
rm -rf ${RESULTS}
mkdir ${RESULTS}
#
COMPILE_COMMANDS="compile_commands.json"
cp ${BUILD}/${COMPILE_COMMANDS} ${SRC}
#
list_of_Check="readability-else-after-return"
#
for file in ${list_of_files}
do
  echo "test for "${file}
  OUTPUT="${RESULTS}/${file}.txt"
  for check in ${list_of_Check}
  do
    clang-tidy -checks="-*, ${check}" -header-filter="./src/*" ${SRC}/${file} \
      > ${OUTPUT} 2>/dev/null
    if [[ -s ${OUTPUT} ]]
    then
      head ${OUTPUT}
    else
      rm -f ${OUTPUT}
    fi
  done
done
#
rm ${SRC}/${COMPILE_COMMANDS}
rmdir --ignore-fail-on-non-empty ${RESULTS}
if [[ -d ${RESULTS} ]]
then
  echo "some problem(s) are still present"
  exit 1
else
  echo "all clang-tidy are OK"
  exit 0
fi
