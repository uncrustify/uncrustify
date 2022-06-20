#
# Toolchain file for cross-compiling from Linux to Win64 using MinGW
#

set(CMAKE_SYSTEM_NAME Windows)

if(NOT COMPILER_PREFIX)
  if(EXISTS /usr/amd64-mingw32msvc-gcc)
    # mingw
    set(COMPILER_PREFIX "amd64-mingw32msvc-gcc")
  else()
    # default to mingw-w64
    set(COMPILER_PREFIX "x86_64-w64-mingw32")
  endif()
endif()

find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
