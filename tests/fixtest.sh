#! /bin/sh
# $Id: difftest.sh 798 2007-07-24 16:01:09Z bengardner $
#
# Copies the files for a test from results/ to output/
#

if [ -z "$1" ] ; then
    fn=$(basename $0)
    echo "Usage: $fn TEST [...]"
    echo
    echo "  TEST : the test number pattern, may contain wildcards"
    echo "         You can put multiple test numbers on the command line"
    echo
    echo "The script will find all matching tests in the results folder and copy them"
    echo "into the output folder."
    echo
    echo "Examples:"
    echo "$fn 30014        # copy test 30014"
    echo "$fn 30014 00110  # copy tests 30014 and 00110"
    echo "$fn '*'          # copy all tests"
    exit 1
fi

while [ -n "$1" ] ; do
    # Use '*' as the pattern if one wasn't defined
    patt=$1
    path="results"

    # Find the tests that match, remove the .svn folders
    files=$(find $path -name "$patt-*" -type f | sed "/\.svn/d")

    did1=''
    for t in $files ; do
        other=$(echo $t | sed "s/^results/output/")
        echo "cp $t $other"
        cp $t $other
    done

    shift 1
done
