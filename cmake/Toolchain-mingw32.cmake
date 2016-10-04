#
# Toolchain file for cross-compiling from Linux to Win32 using MinGW
#

set(CMAKE_SYSTEM_NAME Windows)

if(NOT COMPILER_PREFIX)
  if(EXISTS /usr/i686-w64-mingw32)
    # mingw-w64
    set(COMPILER_PREFIX "i686-w64-mingw32")
  elseif(EXISTS /usr/i586-mingw32msvc)
    # mingw
    set(COMPILER_PREFIX "i586-mingw32msvc")
  else()
    message(FATAL_ERROR "Unable to detect cross-compiler prefix (COMPILER_PREFIX)")
  endif()
endif()

find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${COMPILER_PREFIX})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
