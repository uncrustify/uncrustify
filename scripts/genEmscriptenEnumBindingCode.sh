#!/bin/sh

# see stackoverflow.com/questions/59895
script_dir=$(dirname "$(readlink -f "$0")")

enum0Name="uncrustify_options"
enum1Name="uncrustify_groups"
enum2Name="argtype_e"
enum3Name="log_sev_t"
enum4Name="c_token_t"
enum5Name="lang_flag_e"

enum0SName="Option"
enum1SName="Group"
enum2SName="Argtype"
enum3SName="LogSev"
enum4SName="Token"
enum5SName="LangFlag"

enum0File="$script_dir/../src/options.h"
enum1File="$script_dir/../src/options.h"
enum2File="$script_dir/../src/options.h"
enum3File="$script_dir/../src/log_levels.h"
enum4File="$script_dir/../src/token_enum.h"
enum5File="$script_dir/../src/uncrustify_types.h"

###########################################################################

print_enumCode () {
    # 2 param, 1st not empty, 2nd is file
    if [ "$#" -lt 2 ] || [ "$1" == "" ] || ! [ -f "$2" ]; then
        echo "Usage:" >&2
        echo "   param 1: enum name : string" >&2
        echo "   param 2: file path to the source containing the enum : path" >&2
        exit 1
    fi

    enumName=$1
    filePath=$2

    # remove the first two params from $@
    shift 2

    # extract enum name from clang ast-dump
    enumCutLen=`expr length "$enumName:: "`
    enumValues=`clang-check $@ $filePath -ast-dump \
                    -ast-dump-filter="$enumName::" 2>/dev/null | \
                grep -E -o "$enumName\:\:\w+" | \
                cut -c $enumCutLen-`

    echo -n "   enum_<$enumName>(STRINGIFY($enumName))"
    for option in $enumValues; do
        echo -en "\n      .value(STRINGIFY($option), $option)"
    done
    echo ";"
    echo
}


print_tsCode () {
    if [ "$#" -lt 3 ] || [ "$1" == "" ] || [ "$2" == "" ] || ! [ -f "$3" ]; then
        echo "Usage:"
        echo "   param 1: enum name : string"
        echo "   param 2: file path to the source containing the enum : path"
        echo "   param 3: enum (substitution) interface name"
        exit 1
    fi

    enumName=$1
    subsName=$2
    filePath=$3

    # remove the first three params from $@
    shift 3

    # extract enum name from clang ast-dump
    enumCutLen=`expr length "$enumName:: "`
    enumValues=`clang-check $@ $filePath -ast-dump \
                    -ast-dump-filter="$enumName::" 2>/dev/null | \
                grep -E -o "$enumName\:\:\w+" | \
                cut -c $enumCutLen-`

    echo "    export interface $subsName extends EmscriptenEnumType"
    echo "    {"
    for option in $enumValues; do
        echo "        $option : EmscriptenEnumTypeObject;"
    done
    echo "    }"
    echo
}


if [ "$#" -ne 0 ] && [ "$1" -eq "1" ]; then
    print_tsCode $enum0Name $enum0SName $enum0File
    print_tsCode $enum1Name $enum1SName $enum1File
    print_tsCode $enum2Name $enum2SName $enum2File
    print_tsCode $enum3Name $enum3SName $enum3File
    print_tsCode $enum4Name $enum4SName $enum4File
    print_tsCode $enum5Name $enum5SName $enum5File \
        "-extra-arg=-std=c++1z" \
        "-extra-arg=-DEMSCRIPTEN"
else
    print_enumCode $enum0Name $enum0File
    print_enumCode $enum1Name $enum1File
    print_enumCode $enum2Name $enum2File
    print_enumCode $enum3Name $enum3File
    print_enumCode $enum4Name $enum4File
    print_enumCode $enum5Name $enum5File \
        "-extra-arg=-std=c++1z" \
        "-extra-arg=-DEMSCRIPTEN"
fi
