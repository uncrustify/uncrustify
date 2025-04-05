#! /bin/sh
#
# Author:  guy maurel
# May 2022
#
# generate p-files from the sources
#
# the test is only useful for the developers
#
LIST=$(echo ../src/*.cpp)
LIST_H=$(echo ../src/*.h)
P_FILES="../build/P-files"
#
mkdir -p ${P_FILES}
#
for each in ${LIST} ${LIST_H}
do
  bn=$(basename "${each}")
  echo "${bn}"
  uncrustify -q -c ../forUncrustifySources.cfg -f "${each}" -o /dev/null -p "${P_FILES}/${bn}-p.txt"
done
