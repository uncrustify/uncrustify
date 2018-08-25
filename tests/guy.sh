#! /bin/bash
#
# runs extra tests
#
# guy maurel
# 2018-08-24
#
mkdir -p extras
../build/uncrustify -c config/ben_001.cfg -f input/cpp/cout.cpp -p extras/out.cpp
