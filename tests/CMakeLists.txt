cmake_minimum_required(VERSION 2.8)

find_package(Git QUIET)

set(tests_ctest_file "${CMAKE_CURRENT_BINARY_DIR}/tests.cmake")
set_property(DIRECTORY PROPERTY TEST_INCLUDE_FILE ${tests_ctest_file})

add_custom_command(
  OUTPUT ${tests_ctest_file}
  COMMAND ${PYTHON_EXECUTABLE} run_tests.py
    --executable $<TARGET_FILE:uncrustify>
    --git ${GIT_EXECUTABLE}
    --python ${PYTHON_EXECUTABLE}
    --write-ctest ${tests_ctest_file}
    --cmake-config "$<CONFIGURATION>"
  VERBATIM
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS
    c-sharp.test
    c.test
    cpp.test
    d.test
    ecma.test
    imported.test
    java.test
    objective-c.test
    pawn.test
    vala.test
)

add_custom_target(register_tests ALL DEPENDS ${tests_ctest_file})

if (CMAKE_CONFIGURATION_TYPES)
  # Not really, but just to make the following if() true
  set(_build_type release)
  set(_configs CONFIGURATIONS Release RelWithDebInfo MinSizeRel)
else()
  string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type)
  set(_configs)
endif()

# Only enable test_cli_options stuff if building a release configuration,
# or using a multi-configuration generator (note: in the latter case, these
# will fail if a release configuration has not been built)
if (_build_type MATCHES "release|relwithdebinfo|minsizerel")
  add_test(
    NAME cli_options
    COMMAND ${PYTHON_EXECUTABLE}
      test_cli_options.py
      --build ${uncrustify_BINARY_DIR}
      --cache ${CMAKE_BINARY_DIR}/CMakeCache.txt
      --diff
    ${_configs}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cli
  )

  add_custom_target(update_cli_options
    COMMAND ${PYTHON_EXECUTABLE}
      test_cli_options.py
      --build ${uncrustify_BINARY_DIR}
      --cache ${CMAKE_BINARY_DIR}/CMakeCache.txt
      --apply
    DEPENDS uncrustify
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cli
  )
endif()

add_test(NAME sanity COMMAND uncrustify --help)
