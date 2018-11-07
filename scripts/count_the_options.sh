#! /bin/sh
#
# count the options
# guy maurel
# 6. 11. 2018
#
grep "register_option(" ../build/src/options.cpp > count.UO
wc -l count.UO
rm count.UO
