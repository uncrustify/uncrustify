cmake_minimum_required(VERSION 3.0.0)

# ------------------------------------------------------------------------------


if(DEFINED ENV{EMSCRIPTEN})
    SET(emscripten_root_path "$ENV{EMSCRIPTEN}")
else()
    find_file(EMCC_EXECUTABLE emcc HINTS "/usr/lib/emscripten")
    if(EXISTS ${EMCC_EXECUTABLE})
        get_filename_component(emscripten_root_path ${EMCC_EXECUTABLE} DIRECTORY)
    endif()
endif()

# Abort if not found.
if ("${emscripten_root_path}" STREQUAL "")
  MESSAGE(FATAL_ERROR "Could not locate the Emscripten directory via the \
                       EMSCRIPTEN environment variable! Set it up or pass \
                       -Demscripten_root_path=xxx to CMake to specify the \
                       directory.")
endif()

SET(CMAKE_TOOLCHAIN_FILE "${emscripten_root_path}/cmake/Modules/Platform/Emscripten.cmake")

SET(project_name "libUncrustify")
PROJECT(${project_name})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#get parent folder, add src
GET_FILENAME_COMPONENT(unc_projdir ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)

# ------------------------------------------------------------------------------

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckTypeSize)

find_package(PythonInterp REQUIRED)

#
# Determine config
#
if(WIN32)
  # Windows builds use src/windows_compat.h instead of config.h
else()
  # Generate config.h
  set(avail_headers "")

  set(headers
    inttypes.h
    memory.h
    stdint.h
    stdlib.h
    strings.h
    string.h
    sys/stat.h
    sys/types.h
    unistd.h
    utime.h
  )
  foreach(header ${headers})
    string(TOUPPER "${header}" header_uc)
    string(REGEX REPLACE "[^A-Z0-9_]" "_" include_var "HAVE_${header_uc}")
    check_include_file_cxx("${header}" ${include_var})
    if(${include_var})
      list(APPEND avail_headers ${header})
    endif()
    unset(include_var)
    unset(header_uc)
  endforeach()
  unset(headers)

  check_include_file("stdbool.h" HAVE_STDBOOL_H)

  set(symbols
    memset
    strcasecmp
    strchr
    strdup
    strerror
    strtol
    strtoul
  )
  foreach(symbol ${symbols})
    string(TOUPPER "${symbol}" symbol_uc)
    string(REGEX REPLACE "[^A-Z0-9_]" "_" symbol_var "HAVE_${symbol_uc}")
    check_cxx_symbol_exists("${symbol}" "${avail_headers}" ${symbol_var})
    unset(symbol_var)
    unset(symbol_uc)
  endforeach()
  unset(symbols)

  unset(avail_headers)

  check_type_size(_Bool _BOOL LANGUAGE C)

  configure_file("${unc_projdir}/src/config.h.in" config.h @ONLY)
endif()

#
# Generate uncrustify_version.h
#

set(CURRENT_VERSION "Uncrustify-0.69.0_f")

option(NoGitVersionString "Do not use make_version.py and git to build a version string" OFF)
if(NoGitVersionString)
  configure_file("${unc_projdir}/src/uncrustify_version.h.in" "${PROJECT_BINARY_DIR}/uncrustify_version.h" @ONLY)
else()
  # Add target to generate version header;
  # do this every build to ensure git SHA is up to date
  add_custom_target(generate_version_header
    ${CMAKE_COMMAND}
    -D PYTHON_EXECUTABLE:STRING=${PYTHON_EXECUTABLE}
    -D SOURCE_DIR:PATH="${unc_projdir}"
    -D INPUT:PATH="${unc_projdir}/src/uncrustify_version.h.in"
    -D OUTPUT:PATH="${PROJECT_BINARY_DIR}/uncrustify_version.h"
    -D CURRENT_VERSION:STRING="CURRENT_VERSION"
    -P ${unc_projdir}/cmake/GenerateVersionHeader.cmake
    COMMENT "Generating version header"
  )
  set_source_files_properties(
    "${PROJECT_BINARY_DIR}/uncrustify_version.h"
    PROPERTIES GENERATED TRUE
  )
endif()

#
# Generate token_names.h
#
add_custom_command(
  OUTPUT "${PROJECT_BINARY_DIR}/token_names.h"
  COMMAND ${CMAKE_COMMAND}
    "-Dsrc_file=${unc_projdir}/src/token_enum.h"
    "-Ddst_file=${PROJECT_BINARY_DIR}/token_names.h"
    -P "${unc_projdir}/cmake/GenerateTokenNames.cmake"
  MAIN_DEPENDENCY "${unc_projdir}/src/token_enum.h"
  COMMENT "Generating token_names.h"
)

# Set up commands for generated source files
function(py_gen OUTPUT SCRIPT INPUT)
  set(out "${PROJECT_BINARY_DIR}/src/${OUTPUT}")
  set(deps "${unc_projdir}/src/${INPUT}")
  get_filename_component(outdir "${out}" DIRECTORY)
  foreach(arg IN LISTS ARGN)
    list(APPEND deps "${unc_projdir}/src/${arg}")
  endforeach()

  add_custom_command(
    OUTPUT "${out}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${outdir}"
    COMMAND ${PYTHON_EXECUTABLE}
      "${unc_projdir}/scripts/${SCRIPT}"
      "${out}"
      ${deps}
    DEPENDS ${deps} "${unc_projdir}/scripts/${SCRIPT}"
    MAIN_DEPENDENCY ${unc_projdir}/src/${INPUT}
    COMMENT "Generating ${OUTPUT}"
  )
endfunction()

py_gen(punctuator_table.h
  make_punctuator_table.py
  symbols_table.h
)

py_gen(options.cpp
  make_options.py
  options.h
  options.cpp.in
)

py_gen(option_enum.h
  make_option_enum.py
  option.h
  option_enum.h.in
)

py_gen(option_enum.cpp
  make_option_enum.py
  option.h
  option_enum.cpp.in
)

# ------------------------------------------------------------------------------

FILE(GLOB unc_infiles "${unc_projdir}/src/*.cpp")

ADD_EXECUTABLE(${project_name}
  ${unc_infiles} 
  ${unc_projdir}/src/token_enum.h 
  ${unc_projdir}/src/symbols_table.h 
  ${unc_projdir}/src/options.h
  ${unc_projdir}/src/option.h
  ${PROJECT_BINARY_DIR}/src/options.cpp
  ${PROJECT_BINARY_DIR}/src/option_enum.cpp
  ${PROJECT_BINARY_DIR}/src/option_enum.h
  ${PROJECT_BINARY_DIR}/uncrustify_version.h
  ${unc_projdir}/src/option_enum.cpp.in
  ${unc_projdir}/src/option_enum.h.in
  ${unc_projdir}/src/options.cpp.in
)
add_dependencies(${project_name} generate_version_header)

include_directories(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_BINARY_DIR}/src
  ${unc_projdir}/src
)

SET(unc_compile_flags "--bind -O3")
SET_TARGET_PROPERTIES(${project_name}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        COMPILE_FLAGS "${unc_compile_flags}"
        LINK_FLAGS "${unc_compile_flags} \
                    -s TOTAL_MEMORY=67108864 \
                    -s ALLOW_MEMORY_GROWTH=1 \
                    -s VERBOSE=1 \
                    -s MODULARIZE=1 \
                    -s EXPORT_NAME=\"'${project_name}'\" \
                    -s ERROR_ON_UNDEFINED_SYMBOLS=1 \
                    -s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"UTF8ToString\",\"stringToUTF8\",\"lengthBytesUTF8\", \"writeAsciiToMemory\"]' \
                    -s WASM=0\
                    --memory-init-file 0\
                    --pre-js ${CMAKE_CURRENT_LIST_DIR}/prefix_module.js \
                    --post-js ${CMAKE_CURRENT_LIST_DIR}/postfix_module.js \
                    ")

EM_LINK_PRE_JS(${project_name} ${CMAKE_CURRENT_LIST_DIR}/prefix_module.js)
EM_LINK_POST_JS(${project_name} ${CMAKE_CURRENT_LIST_DIR}/postfix_module.js)

# ------------------------------------------------------------------------------

# add file post/pre-fix
add_custom_command(TARGET ${project_name}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -Dunc_targetfile:FILEPATH=$<TARGET_FILE:${project_name}>
                                            -P "${CMAKE_CURRENT_LIST_DIR}/CMake_catFiles.cmake"
)

add_custom_target(emscripten_test
    COMMAND ${CMAKE_COMMAND} -Dunc_targetfile:FILEPATH=$<TARGET_FILE:${project_name}>
                             -Dunc_projdir:FILEPATH=${unc_projdir}
                             -P "${CMAKE_CURRENT_LIST_DIR}/CMake_emscripten_test.cmake"
    COMMENT "Starting libUncrustify.js tests:"
)
