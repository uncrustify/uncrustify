#
# Toolchain file for cross-compiling from Linux to Win32 using MinGW
#

set(CMAKE_SYSTEM_NAME Windows)

if(NOT COMPILER_PREFIX)
  if(EXISTS /usr/i586-mingw32msvc)
    # mingw
    set(COMPILER_PREFIX "i586-mingw32msvc")
  else()
    # default to mingw-w64
    set(COMPILER_PREFIX "i686-w64-mingw32")
  endif()
endif()

find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
