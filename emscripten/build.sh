#!/bin/sh
# script to build without cmake,
# manually generate header files usually created by CMake,
# past those inside Uncrustifys 'src' dir
# run this script in the 'emscripten' dir

sh_dir="$(dirname "$(readlink -f "$0")")"
outTmp="temp.bak"
out="libUncrustify.js"

#https://github.com/kripken/emscripten/blob/master/src/settings.js

em++ -O3                                                                       \
    ${COMMENT# initialy increase memory for big input files }                  \
    -s TOTAL_MEMORY=67108864                                                   \
    ${COMMENT# let the memory grow dynamically if even more is needed  }       \
    -s ALLOW_MEMORY_GROWTH=1                                                   \
    -s VERBOSE=1                                                               \
    -s MODULARIZE=1                                                            \
    -s EXPORT_NAME="'libUncrustify'"                                           \
    -s ERROR_ON_UNDEFINED_SYMBOLS=1                                            \
    -s ALLOW_MEMORY_GROWTH=1                                                   \
    -s "EXTRA_EXPORTED_RUNTIME_METHODS=['UTF8ToString', 'stringToUTF8', 'lengthBytesUTF8', 'writeAsciiToMemory']" \
    --bind                                                                     \
    --pre-js prefix_module.js                                                  \
    --post-js postfix_module.js                                                \
    ${COMMENT# TODO: handle async ajax load to enable this }                   \
    --memory-init-file 0                                                       \
    -o $out                                                                    \
    ../src/*.cpp                                                               \
&& cat "./prefix_file" "$out" "./postfix_file" > "$outTmp"                     \
&& mv "$outTmp" "$out"                                                         \
&& ./test/run_tests.py "$sh_dir/libUncrustify.js" "$sh_dir/test"
