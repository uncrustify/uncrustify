#!/bin/sh 

outTmp="temp.bak"
out="libUncrustify.js"
cmt="/** 
 * @file libUncrustify.js
 * JS port of Uncrustify
 *
 * @author  Ben Gardner, 
 *          ported by Daniel Chumak with the help of emscripten
 * @license GPLv2
 */"

#https://github.com/kripken/emscripten/blob/master/src/settings.js
 
emcc -O3 \
    ${COMMENT# initialy increase memory for big input files } \
    -s TOTAL_MEMORY=67108864 \
    ${COMMENT# let the memory grow dynamically if even more is needed  } \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s VERBOSE=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="'libUncrustify'" \
    -s ERROR_ON_UNDEFINED_SYMBOLS=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    --bind \
    --pre-js emscripten_module.js \
    --post-js postfix.js \
    ${COMMENT# TODO: handle async ajax load to enable this } \
    --memory-init-file 0 \
    -o $out \
    ../*.cpp \
&& echo "$cmt" | cat - "$out" > "$outTmp" && mv "$outTmp" "$out"
