#!/bin/sh

# see stackoverflow.com/questions/59895
script_dir=$(dirname "$(readlink -f "$0")")

enum0Name="uncrustify_options"
enum1Name="uncrustify_groups"
enum2Name="argtype_e"
enum3Name="log_sev_t"

enum0File="$script_dir/../src/options.h"
enum1File="$script_dir/../src/options.h"
enum2File="$script_dir/../src/options.h"
enum3File="$script_dir/../src/log_levels.h"

###############################################################################

print_enumCode () {
    # 2 param, 1st not empty, 2nd is file
    if [ "$#" -ne 2 ] || [ "$1" == "" ] || ! [ -f "$2" ]; then
        echo "Usage:" >&2
        echo "   param 1: enum name : string" >&2
        echo "   param 2: file path to the source containing the enum : path" >&2
        exit 1
    fi
    
    # extract enum name from clang ast-dump
    enumCutLen=`expr length "$1:: "`
    enumValues=`clang-check $2 -ast-dump -ast-dump-filter="$1::" 2>/dev/null | grep -E -o "$1\:\:\w+" | cut -c $enumCutLen-`
   
    echo -n "   enum_<$1>(STRINGIFY($1))"
    for option in $enumValues; do
        if [ "$option" != "UO_option_count" ]; then
            echo -en "\n      .value(STRINGIFY($option), $option)"
        fi
    done
    echo ";"
    echo
}

print_enumCode $enum0Name $enum0File
print_enumCode $enum1Name $enum1File
print_enumCode $enum2Name $enum2File
print_enumCode $enum3Name $enum3File

