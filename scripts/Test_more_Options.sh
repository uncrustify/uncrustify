#!/bin/bash
#
# @ guy maurel
# 28. 11. 2016
#
# It is not enought to test how uncrustify is running with lot of examples.
# It is necessary to test if uncrustify can run properly.
# The last changes of code (November 2016) show some more problems.
# So it is necessary to test some more.
# It might be usefull to complete the list below.
#
#set -x
SCRIPTS="./scripts"
RESULTS="./results"
#
mkdir -p ${RESULTS}
rm -rf ${RESULTS}/*
#
# Test help
#   -h -? --help --usage
  file="help.txt"
  ./build/uncrustify > ${RESULTS}/${file} 
  cmp -s ${RESULTS}/${file} ${SCRIPTS}/More_Options_to_Test/${file} 
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with "${file}
    echo "use: diff ${RESULTS}/${file} ${SCRIPTS}/Test_more/${file} to find why"
  else
    rm results/${file}
  fi

# Debug Options:
#   -p TODO
#   -L
# look at src/log_levels.h
INPUT="scripts/Input"
OUTPUT="scripts/Output"
#set -x
Liste_of_Ls_A="9 21 25 28 31 36 66 92"
for L_Value in ${Liste_of_Ls_A}
do
  InputFile="${INPUT}/${L_Value}.cpp"
  OutputFile="${OUTPUT}/${L_Value}.txt"
  LFile="${RESULTS}/${L_Value}.txt"
  ./build/uncrustify -c /dev/null -f ${InputFile} -o /dev/null -L ${L_Value} 2> ${LFile}
  sed 's/[0-9]/x/g' ${LFile} > ${LFile}.sed 
  cmp -s ${LFile}.sed ${OutputFile}
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with "${file}
    echo "use: diff ${LFile}.sed ${OutputFile} to find why"
    diff ${LFile} ${OutputFile}
    break
  else
    rm ${LFile}
    rm ${LFile}.sed
  fi
done

rmdir --ignore-fail-on-non-empty results
if [[ -d results ]]
then
  echo "some problem(s) are still present"
  exit 1
else
  echo "all tests are OK"
  exit 0
fi
