#! /bin/sh

if ! [ -d results ]; then
    mkdir results
fi

python run_tests.py *.test

