#! /bin/bash
#
# runs extra tests
#
# guy maurel
#
cd /home/guy/Software/uncrustify/build
#
mkdir -p extras
./uncrustify -c ../tests/config/ben_001.cfg -f ../tests/input/cpp/cout.cpp -p extras/out.cpp
