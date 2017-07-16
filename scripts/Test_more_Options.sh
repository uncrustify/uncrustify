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
#exit 0
SCRIPTS="./scripts"
RESULTS="./results"
#
# control the CMAKE_BUILD_TYPE
CMAKE_BUILD_TYPE=`grep -i CMAKE_BUILD_TYPE:STRING=release ./build/CMakeCache.txt`
how_different=${?}
if [ ${how_different} == "0" ] ;
then
  echo "CMAKE_BUILD_TYPE is correct"
else
  echo "CMAKE_BUILD_TYPE must be 'Release' to test"
  exit 1
fi
#
rm -rf ${RESULTS}
mkdir ${RESULTS}

INPUT="scripts/Input"
OUTPUT="scripts/Output"
CONFIG="scripts/Config"

#
# Test help
#   -h -? --help --usage
file="help.txt"

./build/uncrustify > "${RESULTS}/${file}"
cmp -s "${RESULTS}/${file}" "${SCRIPTS}/More_Options_to_Test/${file}"
how_different=${?}
if [ ${how_different} != "0" ] ;
then
  echo
  echo "Problem with "${file}
  echo "use: diff ${RESULTS}/${file} ${SCRIPTS}/More_Options_to_Test/${file} to find why"
  diff --unified=5 "${RESULTS}/${file}" "${SCRIPTS}/More_Options_to_Test/${file}"
else
  rm "results/${file}"
fi

#
# Test --show-config
#
file="show_config.txt"

./build/uncrustify --show-config > "${RESULTS}/${file}"
sed 's/# Uncrustify.*//g' "${RESULTS}/${file}" > "${RESULTS}/${file}.sed"
cmp -s "${RESULTS}/${file}.sed" "${SCRIPTS}/More_Options_to_Test/${file}"
how_different=${?}
if [ ${how_different} != "0" ] ;
then
  echo
  echo "Problem with ${RESULTS}/${file}.sed"
  echo "use: diff ${RESULTS}/${file}.sed ${SCRIPTS}/More_Options_to_Test/${file} to find why"
  diff "${RESULTS}/${file}.sed" "${SCRIPTS}/More_Options_to_Test/${file}"
else
  rm "results/${file}"
  rm "results/${file}.sed"
fi

#
# Test --update-config
#
ConfigFileNames="mini_d mini_nd"
for ConfigFileName in ${ConfigFileNames}
do
  ResultsFile="${RESULTS}/${ConfigFileName}_uc.txt"
  OutputFile="${OUTPUT}/${ConfigFileName}_uc.txt"
  ConfigFile="${CONFIG}/${ConfigFileName}.cfg"

  ./build/uncrustify -c "${ConfigFile}" --update-config &> "${ResultsFile}"
  sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"
  cmp -s "${ResultsFile}.sed" "${OutputFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with ${ResultsFile}.sed"
    echo "use: diff ${ResultsFile}.sed ${OutputFile} to find why"
    diff "${ResultsFile}.sed" "${OutputFile}"
  else
    rm "${ResultsFile}"
    rm "${ResultsFile}.sed"
  fi
done

#
# Test --update-config-with-doc
#
ConfigFileNames="mini_d mini_nd"
for ConfigFileName in ${ConfigFileNames}
do
  ResultsFile="${RESULTS}/${ConfigFileName}_ucwd.txt"
  OutputFile="${OUTPUT}/${ConfigFileName}_ucwd.txt"
  ConfigFile="${CONFIG}/${ConfigFileName}.cfg"

  ./build/uncrustify -c "${ConfigFile}" --update-config-with-doc &> "${ResultsFile}"
  sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"
  cmp -s "${ResultsFile}.sed" "${OutputFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with ${ResultsFile}.sed"
    echo "use: diff ${ResultsFile}.sed ${OutputFile} to find why"
    diff "${ResultsFile}.sed" "${OutputFile}"
  else
    rm "${ResultsFile}"
    rm "${ResultsFile}.sed"
  fi
done

#
# Test -p
#
ResultsFile="${RESULTS}/p.txt"
InputFile="${INPUT}/28.cpp"
OutputFile="${OUTPUT}/p.txt"
ConfigFile="${CONFIG}/mini_nd.cfg"

./build/uncrustify -c "${ConfigFile}" -f "${InputFile}" -p "${ResultsFile}" &> /dev/null
sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"
cmp -s "${ResultsFile}.sed" "${OutputFile}"
how_different=${?}
if [ ${how_different} != "0" ] ;
then
  echo "Problem with ${ResultsFile}.sed"
  echo "use: diff ${ResultsFile}.sed ${OutputFile} to find why"
  diff "${ResultsFile}.sed" "${OutputFile}"
else
  rm "${ResultsFile}"
  rm "${ResultsFile}.sed"
fi


# Debug Options:
#   -L
# look at src/log_levels.h

Liste_of_Ls_A="9 21 25 28 31 36 66 92"
for L_Value in ${Liste_of_Ls_A}
do
  InputFile="${INPUT}/${L_Value}.cpp"
  OutputFile="${OUTPUT}/${L_Value}.txt"
  LFile="${RESULTS}/${L_Value}.txt"
  ./build/uncrustify -c /dev/null -f "${InputFile}" -o /dev/null -L "${L_Value}" 2> "${LFile}"
  sed 's/[0-9]//g' "${LFile}" > "${LFile}.sed"
  cmp -s "${LFile}.sed" "${OutputFile}"
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with "${InputFile}
    echo "use: diff ${LFile}.sed ${OutputFile} to find why"
    diff "${LFile}.sed" "${OutputFile}"
    diff "${LFile}" "${OutputFile}"
    break
  else
    rm "${LFile}"
    rm "${LFile}.sed"
  fi
done

Liste_of_Error_Tests="I-842 unmatched_close_pp"
for Error_T in ${Liste_of_Error_Tests}
do
  ConfigFile="${CONFIG}/${Error_T}.cfg"
  InputFile="${INPUT}/${Error_T}.cpp"
  OutputFile="${OUTPUT}/${Error_T}.txt"
  ErrFile="${RESULTS}/${Error_T}.txt"
  ./build/uncrustify -q -c "${ConfigFile}" -f "${InputFile}" -o /dev/null 2> "${ErrFile}"
  cmp -s "${ErrFile}" "${OutputFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with "${Error_T}
    echo "use: diff ${ErrFile} ${OutputFile} to find why"
    diff "${ErrFile}" "${OutputFile}"
    break
  else
    rm "${ErrFile}"
  fi
done

rmdir --ignore-fail-on-non-empty results
if [[ -d results ]]
then
  echo
  echo "some problem(s) are still present"
  exit 1
else
  echo "all tests are OK"
  exit 0
fi
