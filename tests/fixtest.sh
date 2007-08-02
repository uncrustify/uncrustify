#! /bin/sh
# $Id: difftest.sh 798 2007-07-24 16:01:09Z bengardner $
#
# Copies the files for a test from results/ to output/
#

if [ -z "$1" ] ; then
    fn=$(basename $0)
    echo "Usage: $fn TEST [LANG]"
    echo
    echo "  TEST : the test number pattern, may contain wildcards"
    echo "  LANG : the language folder name: c, cpp, cs, d, java, oc, pawn, sql"
    echo
    echo "The script will find all matching tests in the results folder and copy them"
    echo "into the output folder."
    echo
    echo "Examples:"
    echo "$fn 30014      # copy test 30014"
    echo "$fn '*' cpp    # copy all CPP tests"
    exit 1
fi

# Use '*' as the pattern if one wasn't defined
patt=$1
if [ -z "$patt" ] ; then
    patt="*"
fi
path="results"
if [ -n "$2" ] ; then
    path="$path/$2"
fi

# Find the tests that match, remove the .svn folders
files=$(find $path -name "$patt-*" -type f | sed "/\.svn/d")

did1=''
for t in $files ; do
    other=$(echo $t | sed "s/^results/output/")
    echo "cp $t $other"
    cp $t $other
done

