cmake_minimum_required(VERSION 3.0.0)

# ------------------------------------------------------------------------------

SET(emscripten_root_path "$ENV{EMSCRIPTEN}")

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

# FIXME: checking for CMAKE_BUILD_TYPE and passing it to make_version.py
# does not work with VS .sln projects
if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Release )
endif()
set( FALLBACK_VERSION "Uncrustify-0.66.1_f" )

find_package(PythonInterp REQUIRED)
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} "${unc_projdir}/scripts/make_version.py" ${CMAKE_BUILD_TYPE}
  WORKING_DIRECTORY ${unc_projdir}
  RESULT_VARIABLE make_version_error
  OUTPUT_VARIABLE make_version_output
)

if (make_version_error)
    message(WARNING "${make_version_error}: ${make_version_output}, ../scripts/make_version.py doesn't work properly")
    set( CURRENT_VERSION ${FALLBACK_VERSION} )
else()
    string(STRIP ${make_version_output} CURRENT_VERSION)
endif()

configure_file("${unc_projdir}/src/uncrustify_version.h.in" uncrustify_version.h @ONLY)


#
# Generate token_names.h
#
add_custom_command(
  OUTPUT "token_names.h"
  COMMAND ${CMAKE_COMMAND}
    "-Dsrc_file=${unc_projdir}/src/token_enum.h"
    "-Ddst_file=token_names.h"
    -P "${unc_projdir}/cmake/GenerateTokenNames.cmake"
  MAIN_DEPENDENCY "${unc_projdir}/src/token_enum.h"
  COMMENT "Generating token_names.h"
)

# ------------------------------------------------------------------------------

FILE(GLOB unc_infiles "${unc_projdir}/src/*.cpp")

ADD_EXECUTABLE(${project_name} ${unc_infiles} ${unc_projdir}/src/token_enum.h)

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
