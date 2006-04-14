#! /bin/sh
#
# Scans src/options.h and sees if any enum items aren't in the table
#
# $Id$
#

in_file="src/options.h"
tmp_file="settings.txt"

bad_file=""

#
# Make sure the OPTDEF stuff is sorted
#
optdef=$(grep "^[ ]*OPTDEF(" $in_file | sed -e 's/,.*//g' -e 's/OPTDEF(//g')

echo -n > $tmp_file
for item in $optdef ; do
    echo "$item" >> $tmp_file
done

echo "Checking $in_file..."
echo

if ( sort -c $tmp_file ); then
    echo "OPTDEF() entries are sorted"
else
    echo "ERROR: OPTDEF() entries are NOT sorted"
    bad_file="Y"
fi

echo

#
# Make sure every UO_ has a OPTDEF()
#

names=$(grep "^[ ]*UO_" $in_file | sed -e 's/,.*//g' -e 's/UO_//g')

for item in $names ; do
    if [ "option_count" != "$item" ] ; then
	match=$( echo "$optdef" | grep "$item" )
	if [ -z "$match" ] ; then
	    echo "ERROR: No OPTDEF() entry for $item"
	    bad_file="Y"
	fi
    fi
done

if [ -n "$bad_file" ] ; then
    echo
    echo "File has problems - fix it"
else
    echo "File looks good"
fi

