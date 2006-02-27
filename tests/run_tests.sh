#! /bin/sh
#
# Create the results folder and then run the python script.
#

if ! [ -d results ]; then
    mkdir results
fi

python run_tests.py *.test

