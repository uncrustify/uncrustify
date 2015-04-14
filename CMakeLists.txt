cmake_minimum_required(VERSION 2.8.3)

project(uncrustify)

include(ExternalProject)

if(NOT WIN32)
  ExternalProject_Add(
    external_uncrustify
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/configure"
      "--prefix" "${CMAKE_CURRENT_BINARY_DIR}"
    BUILD_COMMAND "${MAKE}"
  )

  install(
    PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/bin/uncrustify"
    DESTINATION bin
  )

else()
  set(build_type "${CMAKE_BUILD_TYPE}")
  if("${build_type} " STREQUAL " ")
    set(build_type "Debug")
  endif()

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/uncrustify_version.h"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/create_uncrustify_version_header.bat"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
  )

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/win32/${build_type}/Uncrustify.exe"
    COMMAND "devenv" "/upgrade" "${CMAKE_CURRENT_SOURCE_DIR}/win32/Uncrustify_Win32_2011.sln"
    COMMAND "msbuild" "${CMAKE_CURRENT_SOURCE_DIR}/win32/Uncrustify_Win32_2011.sln" "/p:Configuration=${build_type}" "/p:Platform=Win32"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/uncrustify_version.h"
  )

  add_custom_target(
    external_uncrustify ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/win32/${build_type}/Uncrustify.exe"
  )

  install(
    PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/win32/${build_type}/Uncrustify.exe"
    DESTINATION bin
  )
endif()
