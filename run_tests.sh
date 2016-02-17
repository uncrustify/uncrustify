#! /bin/sh

./detect_Python_2_3.py
howStatus=$?
#
if [ $howStatus != "0" ] ;
then
  exit
fi
cd tests

./run_tests.py $@

exit $?

