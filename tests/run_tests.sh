#! /bin/sh
#
# Create the results folder and then run the python script.
#
# This is dumb - it should be part of the python script.
#

if ! [ -d results ]; then
    mkdir results
fi

python run_tests.py *.test

