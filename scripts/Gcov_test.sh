#!/bin/bash
#
# @author  Guy Maurel
# @license GPL v2+
#
# 30. 4. 2018
#
# The script prepare a new version of uncrustify with the compile options:
#  CMAKE_CXX_FLAGS                  -fprofile-arcs -ftest-coverage
#  CMAKE_C_FLAGS                    -fprofile-arcs -ftest-coverage
# to use the facilities from gcov.
# Running uncrustify with all the test data will mark all parts of the sources
# which are used.
# The "not marked" portions, if any, should give the opportunity to prepare new
# test data to complete the whole tests.
# The results are stored in the directory ${TOTALS_DIR}
# The name of the file is ${source_file}.total
# The line(s) of code which are still not used by any of the tests cases are
# marked which the token "#####" at the beginning of the line.
# As the testing part (unc_tools.cpp, backup.cpp) are only used by a developper,
# all the lines are marked.
# Also the detect.cpp part of uncrustify is completly marked.
#
# TAKE ATTENTION:
# ===============
#
# Running the test is long. I need about 20 minutes.
# This is about 40 times so much as the ctest.
# The disk space necessary is also very big, about 3 Gbytes
# This is about 1500 times bigger as the sources.
#
SCRIPT_NAME=$0
#echo "SCRIPT_NAME="${SCRIPT_NAME}
BASE_NAME=`basename ${SCRIPT_NAME}`
DIR_NAME=`dirname ${SCRIPT_NAME}`
if [ ${DIR_NAME} != "." ] ;
then
  echo "you must use the script at the directory <uncrustify_directory>/scripts"
  exit
fi
cd ..
SOURCES_LIST_H=`ls -1 src/*.h | cut -b 5-`
SOURCES_LIST_CPP=`ls -1 src/*.cpp | cut -b 5-`
#
rm -rf gcov_test
mkdir gcov_test
#
cd gcov_test
# build a new uncrustify binary
cmake -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage" \
      -D CMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage" ..
make
# use uncrustify without parameter
./uncrustify
#
GCNO_LIST=`ls -1 ./CMakeFiles/uncrustify.dir/src/*.gcno`
for gcno_file in ${GCNO_LIST}
do
  echo "gcno_file=${gcno_file}"
  gcno_base_name=`basename ${gcno_file} .gcno`
  echo ${gcno_base_name}
  gcov ${gcno_file} -m
done
#
ADD_TEST_LIST="add_test_list.txt"
ADD_TEST_LIST_10="add_test_list_10.txt"
ADD_TEST_LIST_NUMBER="add_test_list_number.txt"
ADD_TEST_LIST_AWK="../scripts/add_test_list.awk"
ADD_TEST_LIST_CMD="add_test_list.sh"
#
# prepare a list of all tests
grep add_test ../build/tests/CTestTestfile.cmake > ${ADD_TEST_LIST}
cut -b 10- < ${ADD_TEST_LIST} > ${ADD_TEST_LIST_10}
cut --delimiter=" " --fields=1 < ${ADD_TEST_LIST_10} > ${ADD_TEST_LIST_NUMBER}
#
NUMBER_LIST=`cat ${ADD_TEST_LIST_NUMBER}`
#
# prepare a new script file to use uncrustify with all the tests cases
gawk --file ${ADD_TEST_LIST_AWK} \
     --assign sources_cpp="${SOURCES_LIST_CPP}" \
     --assign sources_h="${SOURCES_LIST_H}" < ${ADD_TEST_LIST} > ${ADD_TEST_LIST_CMD}
chmod +x ${ADD_TEST_LIST_CMD}
#
# ATTENTION: this takes about 10 minutes
# use the new script file ADD_TEST_LIST_CMD to build the information
./${ADD_TEST_LIST_CMD}
#
# compare, add the counts of each lines of generated gcov-tests
COMPARE_AND_ADD="../scripts/compare_the_gcov.awk"
TOTALS_DIR="Totals"
mkdir -p Totals
#
# choose
DO_IT_WITH_TEST="yes"
#DO_IT_WITH_TEST="no"
#
# and apply
if [ ${DO_IT_WITH_TEST} == "yes" ]
then
  # do it with intermediate files
  # to save the last file of each test
  for test_number in ${NUMBER_LIST}
  do
    last_test_number=${test_number}
  done
  #
  for source_file in ${SOURCES_LIST_CPP}
  do
    echo "source_file is ${source_file}"
    I_file="blabla"
    # this file doesn' exists
    for test_number in ${NUMBER_LIST}
    do
      echo "source_file is ${source_file}: test_number=${test_number}"
      H_DIR="${source_file}_Dir"
      mkdir -p ${TOTALS_DIR}/${H_DIR}
      TEST_FILE=${test_number}/${source_file}.gcov
      if [ -s ${TEST_FILE} ] ;
      then
        O_file="${TOTALS_DIR}/${H_DIR}/${test_number}"
        gawk --file ${COMPARE_AND_ADD} \
             --assign  in_file="${I_file}" \
             --assign out_file="${O_file}" < ${TEST_FILE}
        I_file=${O_file}
      fi
      # to brake before the end
      #if [ "${test_number}" == "c-sharp_10010" ]
      #if [ "${test_number}" == "c_10005" ]
      #if [ "${test_number}" == "cpp_60042" ]
      #then
      #  exit
      #fi
    done
    # save the last file of each test
    cp ${O_file} ${TOTALS_DIR}/${source_file}.total
  done
else
  # do it directly, without intermediate files
  for source_file in ${SOURCES_LIST_CPP}
  do
    for test_number in ${NUMBER_LIST}
    do
      echo "source_file is ${source_file}: test_number=${test_number}"
      TEST_FILE=${test_number}/${source_file}.gcov
      TOTALS_FILE=${source_file}
      if [ -s ${TEST_FILE} ] ;
      then
        gawk --file ${COMPARE_AND_ADD} \
             --assign  in_file="${TOTALS_DIR}/${TOTALS_FILE}" \
             --assign out_file="${TOTALS_DIR}/${TOTALS_FILE}" < ${TEST_FILE}
      fi
      # to brake before the end
      #if [ "${test_number}" == "c-sharp_10010" ]
      #if [ "${test_number}" == "c_10005" ]
      #if [ "${test_number}" == "cpp_60042" ]
      #then
      #  exit
      #fi
    done
  done
fi
