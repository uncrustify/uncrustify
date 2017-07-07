#! /bin/sh
#
# count the options
# guy maurel
# 5. 10. 2016
#
grep "UO_" ../src/options.h > count.UO
gawk -f count.awk < count.UO
rm count.UO
