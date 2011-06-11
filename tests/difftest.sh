#! /bin/sh
# Shows the difference for a failed test
#

if [ -n "$1" ] ; then
    case  "$1" in
        -h|--help|--usage)
            fn=$(basename $0)
            echo "Usage: $fn [TEST [LANG]]"
            echo
            echo "  TEST : the test number pattern, may contain wildcards"
            echo "  LANG : the language folder name: c, cpp, cs, d, java, oc, pawn, sql"
            echo
            echo "The script will find all matching tests in the output folder and diff them against the same"
            echo "file in the results folder.  The language is only required if the same test number is used"
            echo "for multiple languages and you only want to see one of them."
            echo
            echo "Examples:"
            echo "$fn            # show all test failures"
            echo "$fn 30014      # show test failure 30014"
            echo "$fn '*' cpp    # show all CPP test failure"
            exit 1
	    ;;
    esac
fi

# Use '*' as the pattern if one wasn't defined
patt=$1
if [ -z "$patt" ] ; then
    patt="*"
fi
path="output"
if [ -n "$2" ] ; then
    path="$path/$2"
fi

# Find the tests that match, remove the .svn folders
files=$(find $path -name "$patt-*" -type f | sed "/\.svn/d")

did1=''
for t in $files ; do
    other=$(echo $t | sed "s/^output/results/")
    diff -u $t $other
    if [ "$?" = "1" ] ; then
       did1='yup'
    fi
done

if [ -z "$did1" ] ; then
    echo "No differences"
fi
