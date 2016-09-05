cmake_minimum_required(VERSION 2.8)

project(uncrustify)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "
    In-source builds are not supported, please remove the `CMakeFiles'
    folder and `CMakeCache.txt', and create a folder for the build:
    mkdir build; cd build; cmake ..
  ")
endif()

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckTypeSize)
include(CTest)

#
# Check compiler flags
#
if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /wd4267)
elseif(CMAKE_COMPILER_IS_GNUCXX)
  set(gcc_warning_flags
    -Wall
    -Wextra
    -Wshadow
    -Wpointer-arith
    -Wcast-qual
    -Wcast-align
  )
  foreach(flag ${gcc_warning_flags})
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" flag_var "CXXFLAG_${flag}")
    CHECK_CXX_COMPILER_FLAG("${flag}" ${flag_var})
    if(${flag_var})
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
    endif()
    unset(flag_var)
  endforeach()
  unset(gcc_warning_flags)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if(ENABLE_SANITIZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fno-omit-frame-pointer -fsanitize=${ENABLE_SANITIZER}")
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
# Determine config
#
if(WIN32)
  # Windows builds use win32/windows_compat.h instead of config.h
  include_directories(${PROJECT_SOURCE_DIR}/win32)
else()
  # Generate CMake config.h.in
  file(READ src/config.h.in config_lines)
  string(REPLACE "#undef" "#cmakedefine" config_lines "${config_lines}")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h.in ${config_lines})

  unset(config_lines)

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

  configure_file(${CMAKE_CURRENT_BINARY_DIR}/config.h.in config.h @ONLY)
endif()

#
# Generate uncrustify_version.h
#
file(STRINGS configure.ac PACKAGE_VERSION REGEX "^AC_INIT")

if(${PACKAGE_VERSION} MATCHES "^AC_INIT\\(uncrustify,([^,]+),")
  set(PACKAGE_VERSION "${CMAKE_MATCH_1}")
  message(STATUS "PACKAGE_VERSION: ${PACKAGE_VERSION}")
else()
  message(FATAL_ERROR "Unable to determine PACKAGE_VERSION")
endif()

configure_file(src/uncrustify_version.h.in uncrustify_version.h @ONLY)

#
# Generate token_names.h
#
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/token_names.h"
  COMMAND ${CMAKE_COMMAND}
    "-Dsrc_file=${PROJECT_SOURCE_DIR}/src/token_enum.h"
    "-Ddst_file=${CMAKE_CURRENT_BINARY_DIR}/token_names.h"
    -P "${PROJECT_SOURCE_DIR}/cmake/GenerateTokenNames.cmake"
  MAIN_DEPENDENCY src/token_enum.h
  COMMENT "Generating token_names.h"
)

#
# Uncrustify
#
add_executable(uncrustify
  src/align.cpp
  src/align_stack.cpp
  src/args.cpp
  src/backup.cpp
  src/braces.cpp
  src/brace_cleanup.cpp
  src/ChunkStack.cpp
  src/chunk_list.cpp
  src/combine.cpp
  src/compat_posix.cpp
  src/compat_win32.cpp
  src/defines.cpp
  src/detect.cpp
  src/indent.cpp
  src/keywords.cpp
  src/lang_pawn.cpp
  src/logger.cpp
  src/logmask.cpp
  src/md5.cpp
  src/newlines.cpp
  src/options.cpp
  src/options_for_QT.cpp
  src/output.cpp
  src/parens.cpp
  src/parse_frame.cpp
  src/punctuators.cpp
  src/semicolons.cpp
  src/sorting.cpp
  src/space.cpp
  src/tokenize.cpp
  src/tokenize_cleanup.cpp
  src/uncrustify.cpp
  src/unc_text.cpp
  src/unc_tools.cpp
  src/unicode.cpp
  src/universalindentgui.cpp
  src/width.cpp
  src/align_stack.h
  src/args.h
  src/backup.h
  src/base_types.h
  src/char_table.h
  src/ChunkStack.h
  src/chunk_list.h
  src/ListManager.h
  src/logger.h
  src/logmask.h
  src/log_levels.h
  src/md5.h
  src/options.h
  src/options_for_QT.h
  src/prototypes.h
  src/punctuators.h
  src/token_enum.h
  src/uncrustify_types.h
  src/unc_ctype.h
  src/unc_text.h
  src/unc_tools.h
)

if(CMAKE_VERSION VERSION_LESS 2.8.10)
  if(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
    # Multi-configuration or build type set
    set_property(TARGET uncrustify APPEND PROPERTY
      COMPILE_DEFINITIONS_DEBUG DEBUG
    )
  else()
    # Single-configuration with no build type set
    set_property(TARGET uncrustify APPEND PROPERTY
      COMPILE_DEFINITIONS DEBUG
    )
  endif()
else()
  set_property(TARGET uncrustify APPEND PROPERTY
    COMPILE_DEFINITIONS $<$<OR:$<CONFIG:Debug>,$<CONFIG:>>:DEBUG>
  )
endif()

#
# Generate uncrustify.1
#
configure_file(man/uncrustify.1.in uncrustify.1 @ONLY)

#
# Tests
#
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

#
# Package
#
set(CPACK_PACKAGE_NAME "uncrustify")
set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}")
set(CPACK_PACKAGE_VENDOR "Ben Gardner")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Code beautifier")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/;/\\\\.hg/;/tests/results/;/build.*/")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
  set(CPACK_GENERATOR "ZIP")
endif()
include(CPack)

#
# Install
#
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  install(TARGETS uncrustify DESTINATION ".")
  install(
    FILES
      "${PROJECT_SOURCE_DIR}/README.md"
      "${PROJECT_SOURCE_DIR}/BUGS"
      "${PROJECT_SOURCE_DIR}/ChangeLog"
    DESTINATION "."
  )
  install(FILES "${PROJECT_SOURCE_DIR}/documentation/htdocs/index.html"
    DESTINATION "doc"
  )
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/etc/"
    DESTINATION "cfg"
    FILES_MATCHING PATTERN "*.cfg"
  )
else()
  include(GNUInstallDirs)
  install(TARGETS uncrustify
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/uncrustify.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
  )
endif()
