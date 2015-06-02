cmake_minimum_required(VERSION 2.8.3)

project(uncrustify)

include(ExternalProject)

if(NOT WIN32)
  set(uncrustify_source_dir "${CMAKE_CURRENT_BINARY_DIR}/external_project/src/uncrustify")

  # create non-empty source folder in binary directory
  # otherwise the external projects fails since it does not have a download command
  file(MAKE_DIRECTORY "${uncrustify_source_dir}")
  file(WRITE "${uncrustify_source_dir}/non-empty-folder")

  # add a standard autotools external project
  ExternalProject_Add(
    uncrustify
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external_project"
    CONFIGURE_COMMAND "${uncrustify_source_dir}/configure"
      "--prefix" "${CMAKE_CURRENT_BINARY_DIR}/external_project"
    BUILD_COMMAND "make"
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND "echo" "Skipping install step of external project"
  )

  # always copy the necessary source files of uncrustify to the build directory
  # since the upstream build scripts can't handle symlinked folders correctly
  set(copy_commands "")
  file(GLOB_RECURSE source_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/configure"
    "${CMAKE_CURRENT_SOURCE_DIR}/etc/*.cfg"
    "${CMAKE_CURRENT_SOURCE_DIR}/install-sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/make_token_names.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/man/uncrustify.1.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/missing"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*"
  )
  foreach(source_file ${source_files})
    # copy every file only when changed which will avoid recompilation if nothing changed
    list (APPEND copy_commands
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}"
          "${uncrustify_source_dir}/${source_file}")
  endforeach()
  ExternalProject_Add_Step(uncrustify copy_source
    # it must happen before the configure step
    DEPENDERS configure
    COMMENT "Copy source files to '${uncrustify_source_dir}'"
    # it must happen every time to ensure that changes propagate
    # even if that retriggers configure everytime
    ALWAYS 1
    ${copy_commands}
  )

  install(
    PROGRAMS "${uncrustify_source_dir}/src/uncrustify"
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
