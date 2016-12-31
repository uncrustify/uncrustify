#!/bin/sh
#https://github.com/kripken/sql.js/blob/master/test/run.sh

if [ "$#" -lt 1 ] || ! [ -f "$1" ] || ! [ -d "$2" ]; then
    echo "Usage:" >&2
    echo "   arg 1: libUncrustify.js file path" >&2
    echo "   arg 2: test directory path" >&2
    exit 1
fi

passed=0
total=0
testfiles="$(dirname "$(readlink -f "$0")")/test_*.js"
for f in $testfiles
do
	total=`expr $total + 1`
	echo -ne "Testing $f...\t"
	
	node "$f" "$1" "$2" > /tmp/libuncrustifyjs_test
	if [ $? = 0 ]; then
		echo "Passed."
		passed=`expr $passed + 1`
	else
		echo -e "\033[31mFail!\033[0m"
		cat /tmp/libuncrustifyjs_test
	fi
done

if [ $passed = $total ]
then
	echo -e "\033[32mAll $total tests passed\033[0m"
	exit 0
else
	echo -e "\033[31mWarning\033[0m : $passed tests passed out of $total"
	exit 1
fi 
