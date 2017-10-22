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


# jump into the script dir so that printed paths are always the same
init_pwd=$PWD
script_dir=$(dirname "$(readlink -f "$0")")
cd $script_dir

RELATIVE=$(perl -MFile::Spec -e "print File::Spec->abs2rel(q(${script_dir}),q(${init_pwd}))")


#set -x
#exit 0
# control the CMAKE_BUILD_TYPE
CMAKE_BUILD_TYPE=`grep -i CMAKE_BUILD_TYPE:STRING=release ../../build/CMakeCache.txt`
how_different=${?}
if [ ${how_different} == "0" ]; then
  echo "CMAKE_BUILD_TYPE is correct"
else
  echo "CMAKE_BUILD_TYPE must be 'Release' to test"
  exit 1
fi


INPUT="Input"
OUTPUT="Output"
CONFIG="Config"
RESULTS="Results"

rm -rf ${RESULTS}
mkdir ${RESULTS}


#
# Test help
#   -h -? --help --usage
file="help.txt"
ResultsFile="${RESULTS}/${file}"
OutputFile="${OUTPUT}/${file}"

../../build/uncrustify > "${ResultsFile}"
sed -e ':a' -e 'N' -e '$!ba' -e 's| --mtime      : Preserve mtime on replaced files.\n||g' "${ResultsFile}" > "${ResultsFile}.sed"

cmp -s "${ResultsFile}.sed" "${OutputFile}"
how_different=${?}
if [ ${how_different} != "0" ] ;
then
  echo
  echo "Problem with ${ResultsFile}.sed"
  echo "use: diff ${RELATIVE}/${ResultsFile}.sed ${RELATIVE}/${OutputFile} to find out why"
  diff "${ResultsFile}.sed" "${OutputFile}"
  echo
else
  rm "${ResultsFile}"
  rm "${ResultsFile}.sed"
fi

#
# Test --show-config
#
file="show_config.txt"
ResultsFile="${RESULTS}/${file}"
OutputFile="${OUTPUT}/${file}"

../../build/uncrustify --show-config > "${ResultsFile}"
sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"

cmp -s "${ResultsFile}.sed" "${OutputFile}"
how_different=${?}
if [ ${how_different} != "0" ] ;
then
  echo
  echo "Problem with ${ResultsFile}.sed"
  echo "use: diff ${RELATIVE}/${ResultsFile}.sed ${RELATIVE}/${OutputFile} to find out why"
  diff "${ResultsFile}.sed" "${OutputFile}"
  echo
else
  rm "${ResultsFile}"
  rm "${ResultsFile}.sed"
fi

#
# Test --update-config
#
G_ErrorFile="mini_d_error"
E_ErrorFile="${OUTPUT}/${G_ErrorFile}.txt"

ConfigFileNames="mini_d mini_nd"
IDX=0
for ConfigFileName in ${ConfigFileNames}; do
  ResultsFile="${RESULTS}/${ConfigFileName}_uc.txt"
  OutputFile="${OUTPUT}/${ConfigFileName}_uc.txt"
  ConfigFile="${CONFIG}/${ConfigFileName}.cfg"
  R_ErrorFile="${RESULTS}/${G_ErrorFile}${IDX}.txt"

  ../../build/uncrustify -c "${ConfigFile}" --update-config > "${ResultsFile}" 2> "${R_ErrorFile}"
  sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"
  cmp -s "${ResultsFile}.sed" "${OutputFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with ${ResultsFile}.sed"
    echo "use: diff ${RELATIVE}/${ResultsFile}.sed ${RELATIVE}/${OutputFile} to find out why"
    diff "${ResultsFile}.sed" "${OutputFile}"
    echo
  else
    rm "${ResultsFile}"
    rm "${ResultsFile}.sed"
  fi

  cmp -s "${R_ErrorFile}" "${E_ErrorFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with ${R_ErrorFile}"
    echo "use: diff ${RELATIVE}/${R_ErrorFile} ${RELATIVE}/${E_ErrorFile} to find out why"
    diff "${R_ErrorFile}" "${E_ErrorFile}"
    echo
  else
    rm "${R_ErrorFile}"
  fi
  IDX=`expr $IDX + 1` 
done

#
# Test --update-config-with-doc
#
ConfigFileNames="mini_d mini_nd"
for ConfigFileName in ${ConfigFileNames}; do
  ResultsFile="${RESULTS}/${ConfigFileName}_ucwd.txt"
  OutputFile="${OUTPUT}/${ConfigFileName}_ucwd.txt"
  ConfigFile="${CONFIG}/${ConfigFileName}.cfg"  
  R_ErrorFile="${RESULTS}/${G_ErrorFile}${IDX}.txt"

  ../../build/uncrustify -c "${ConfigFile}" --update-config-with-doc > "${ResultsFile}" 2> "${R_ErrorFile}"
  sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"
  cmp -s "${ResultsFile}.sed" "${OutputFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo "Problem with ${ResultsFile}.sed"
    echo "use: diff ${RELATIVE}/${ResultsFile}.sed ${RELATIVE}/${OutputFile} to find out why"
    diff "${ResultsFile}.sed" "${OutputFile}"
    echo
  else
    rm "${ResultsFile}"
    rm "${ResultsFile}.sed"
  fi

  cmp -s "${R_ErrorFile}" "${E_ErrorFile}"
  how_different=${?}
  if [ ${how_different} != "0" ]; then
    echo
    echo "Problem with ${R_ErrorFile}"
    echo "use: diff ${RELATIVE}/${R_ErrorFile} ${RELATIVE}/${E_ErrorFile} to find out why"
    diff "${R_ErrorFile}" "${E_ErrorFile}"
    echo
  else
    rm "${R_ErrorFile}"
  fi
  IDX=`expr $IDX + 1` 
done

#
# Test -p
#
ResultsFile="${RESULTS}/p.txt"
InputFile="${INPUT}/testSrc.cpp"
OutputFile="${OUTPUT}/p.txt"
ConfigFile="${CONFIG}/mini_nd.cfg"

../../build/uncrustify -c "${ConfigFile}" -f "${InputFile}" -p "${ResultsFile}" &> /dev/null
sed 's/# Uncrustify.*//g' "${ResultsFile}" > "${ResultsFile}.sed"

cmp -s "${ResultsFile}.sed" "${OutputFile}"
how_different=${?}
if [ ${how_different} != "0" ] ;
then
  echo
  echo "Problem with ${ResultsFile}.sed"
  echo "use: diff ${RELATIVE}/${ResultsFile}.sed ${RELATIVE}/${OutputFile} to find out why"
  diff "${ResultsFile}.sed" "${OutputFile}"
  echo
else
  rm "${ResultsFile}"
  rm "${ResultsFile}.sed"
fi

# Debug Options:
#   -L
# look at src/log_levels.h
Liste_of_Ls_A="9 21 25 28 31 36 66 92"
for L_Value in ${Liste_of_Ls_A}; do
  InputFile="${INPUT}/testSrc.cpp"
  OutputFile="${OUTPUT}/${L_Value}.txt"
  LFile="${RESULTS}/${L_Value}.txt"

  ../../build/uncrustify -c /dev/null -f "${InputFile}" -o /dev/null -L "${L_Value}" 2> "${LFile}"
  sed 's/[0-9]//g' "${LFile}" > "${LFile}.sed"

  cmp -s "${LFile}.sed" "${OutputFile}"
  how_different=${?}
  #echo "the status of is "${how_different}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with ${LFile}.sed"
    echo "use: diff ${RELATIVE}/${LFile}.sed ${RELATIVE}/${OutputFile} to find out why"
    diff "${LFile}.sed" "${OutputFile}"
    diff "${LFile}" "${OutputFile}"
    echo
    break
  else
    rm "${LFile}"
    rm "${LFile}.sed"
  fi
done

Liste_of_Error_Tests="I-842 unmatched_close_pp"
for Error_T in ${Liste_of_Error_Tests}; do
  ConfigFile="${CONFIG}/${Error_T}.cfg"
  InputFile="${INPUT}/${Error_T}.cpp"
  OutputFile="${OUTPUT}/${Error_T}.txt"
  ErrFile="${RESULTS}/${Error_T}.txt"

  ../../build/uncrustify -q -c "${ConfigFile}" -f "${InputFile}" -o /dev/null 2> "${ErrFile}"

  cmp -s "${ErrFile}" "${OutputFile}"
  how_different=${?}
  if [ ${how_different} != "0" ] ;
  then
    echo
    echo "Problem with ${ErrFile}"
    echo "use: diff ${RELATIVE}/${ErrFile} ${RELATIVE}/${OutputFile} to find out why"
    diff "${ErrFile}" "${OutputFile}"
    echo
    break
  else
    rm "${ErrFile}"
  fi
done


rmdir --ignore-fail-on-non-empty ${RESULTS}
if [[ -d ${RESULTS} ]]
then
  echo
  echo "some problem(s) are still present"
  exit 1
else
  echo "all tests are OK"
  exit 0
fi
