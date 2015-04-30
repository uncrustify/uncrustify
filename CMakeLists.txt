cmake_minimum_required(VERSION 2.8.3)

project(uncrustify)

include(ExternalProject)

if(NOT WIN32)
  # Make the destination for the source code.
  set(uncrustify_source_dir "${CMAKE_CURRENT_BINARY_DIR}/uncrustify_src")
  file(MAKE_DIRECTORY "${uncrustify_source_dir}")
  # Touch a file there during configure time to prevent ExternalProject_Add from complaining about
  # SOURCE_DIR being an empty folder.
  execute_process(COMMAND
    ${CMAKE_COMMAND} -E touch "${uncrustify_source_dir}/fool_external_project")
  # Add a standard autotools external project, using the build space version of the source.
  ExternalProject_Add(
    external_uncrustify
    SOURCE_DIR "${uncrustify_source_dir}"
    CONFIGURE_COMMAND "${uncrustify_source_dir}/configure"
      "--prefix" "${CMAKE_CURRENT_BINARY_DIR}"
    BUILD_COMMAND "${MAKE}"
  )
  # Always copy the source of uncrustify to the build space between the download and build steps.
  # This prevents problems that occur when the source space is a symlinked directory.
  # See: https://github.com/ament/uncrustify/issues/1
  ExternalProject_Add_Step(external_uncrustify copy_source
    DEPENDERS configure
    COMMENT "Copy source to ${uncrustify_source_dir} to prevent problems with symlink."
    ALWAYS 1  # Always do this otherwise changing the source would not change the built binary.
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/ ${uncrustify_source_dir}
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
    COMMAND
      "msbuild" "${CMAKE_CURRENT_SOURCE_DIR}/win32/Uncrustify_Win32_2011.sln"
      "/p:Configuration=${build_type}" "/p:Platform=Win32"
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
